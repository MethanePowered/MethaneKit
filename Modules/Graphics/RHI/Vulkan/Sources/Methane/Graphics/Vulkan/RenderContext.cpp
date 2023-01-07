/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Vulkan/RenderContext.cpp
Vulkan implementation of the render context interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/System.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>
#include <Methane/Graphics/Vulkan/Shader.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/RenderPass.h>
#include <Methane/Graphics/Vulkan/RenderState.h>
#include <Methane/Graphics/Vulkan/RenderPattern.h>
#include <Methane/Graphics/Vulkan/Buffer.h>
#include <Methane/Graphics/Vulkan/Texture.h>
#include <Methane/Graphics/Vulkan/Sampler.h>
#include <Methane/Graphics/Vulkan/ICommandList.h>
#include <Methane/Graphics/Vulkan/Platform.h>
#include <Methane/Graphics/Vulkan/Types.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <sstream>

namespace Methane::Graphics::Vulkan
{

#ifndef __APPLE__

RenderContext::RenderContext(const Methane::Platform::AppEnvironment& app_env, Device& device,
                                 tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
    : Context<Base::RenderContext>(device, parallel_executor, settings)
    , m_vk_device(device.GetNativeDevice())
    , m_vk_unique_surface(Platform::CreateVulkanSurfaceForWindow(static_cast<System&>(Rhi::ISystem::Get()).GetNativeInstance(), app_env))
{
    META_FUNCTION_TASK();
}

#endif // #ifndef __APPLE__

RenderContext::~RenderContext()
{
    META_FUNCTION_TASK();
    try
    {
        RenderContext::Release();
    }
    catch(const std::exception& e)
    {
        META_UNUSED(e);
        META_LOG("WARNING: Unexpected error during Query destruction: {}", e.what());
        assert(false);
    }
}

Ptr<Rhi::ICommandQueue> RenderContext::CreateCommandQueue(Rhi::CommandListType type) const
{
    META_FUNCTION_TASK();
    auto command_queue_ptr = std::make_shared<CommandQueue>(*this, type);
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    // Base::TimestampQueryPool construction uses command queue and requires it to be fully constructed
    command_queue_ptr->InitializeTimestampQueryPool();
#endif
    return command_queue_ptr;
}

Ptr<Rhi::IShader> RenderContext::CreateShader(Rhi::ShaderType type, const Rhi::ShaderSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<Shader>(type, *this, settings);
}

Ptr<Rhi::IProgram> RenderContext::CreateProgram(const Rhi::ProgramSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<Program>(*this, settings);
}

Ptr<Rhi::IBuffer> RenderContext::CreateBuffer(const Rhi::BufferSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<Buffer>(*this, settings);
}

Ptr<Rhi::ITexture> RenderContext::CreateTexture(const Rhi::TextureSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<Texture>(*this, settings);
}

Ptr<Rhi::ISampler> RenderContext::CreateSampler(const Rhi::SamplerSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<Sampler>(*this, settings);
}

Ptr<Rhi::IRenderState> RenderContext::CreateRenderState(const Rhi::RenderStateSettings& settings) const
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderState>(*this, settings);
}

Ptr<Rhi::IRenderPattern> RenderContext::CreateRenderPattern(const Rhi::RenderPatternSettings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPattern>(*this, settings);
}

void RenderContext::Release()
{
    META_FUNCTION_TASK();
    ReleaseNativeSwapchainResources();
    Context<Base::RenderContext>::Release();
}

bool RenderContext::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Context::SetName(name))
        return false;

    ResetNativeObjectNames();
    return true;
}

void RenderContext::Initialize(Base::Device& device, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    SetDevice(device);
    InitializeNativeSwapchain();
    UpdateFrameBufferIndex();
    Context<Base::RenderContext>::Initialize(device, is_callback_emitted);
}

void RenderContext::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    Context<Base::RenderContext>::WaitForGpu(wait_for);

    std::optional<Data::Index> frame_buffer_index;
    Rhi::CommandListType cl_type = Rhi::CommandListType::Render;
    switch (wait_for)
    {
    case WaitFor::RenderComplete:    m_vk_device.waitIdle(); break;
    case WaitFor::FramePresented:    frame_buffer_index = GetFrameBufferIndex(); break;
    case WaitFor::ResourcesUploaded: cl_type = Rhi::CommandListType::Transfer; break;
    default: META_UNEXPECTED_ARG(wait_for);
    }

    GetVulkanDefaultCommandQueue(cl_type).CompleteExecution(frame_buffer_index);
}

bool RenderContext::ReadyToRender() const
{
    META_FUNCTION_TASK();
    return true;
}

void RenderContext::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    ReleaseNativeSwapchainResources();

    Context<Base::RenderContext>::Resize(frame_size);

    InitializeNativeSwapchain();
    UpdateFrameBufferIndex();
}

void RenderContext::Present()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::Present");
    Context<Base::RenderContext>::Present();

    auto& render_command_queue = static_cast<CommandQueue&>(GetRenderCommandKit().GetQueue());

    // Present frame to screen
    const uint32_t image_index = GetFrameBufferIndex();
    const vk::PresentInfoKHR present_info(
        render_command_queue.GetWaitForFrameExecutionCompleted(image_index).semaphores,
        GetNativeSwapchain(), image_index
    );
    if (const vk::Result present_result = render_command_queue.GetNativeQueue().presentKHR(present_info);
        present_result != vk::Result::eSuccess &&
        present_result != vk::Result::eSuboptimalKHR)
        throw InvalidArgumentException<vk::Result>("RenderContext::Present", "present_result", present_result, "failed to present frame image on screen");

    render_command_queue.ResetWaitForFrameExecution(image_index);

    Context<Base::RenderContext>::OnCpuPresentComplete();
    UpdateFrameBufferIndex();
}

bool RenderContext::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    if (Base::RenderContext::SetVSyncEnabled(vsync_enabled))
    {
        ResetNativeSwapchain();
        return true;
    }
    return false;
}

bool RenderContext::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    META_FUNCTION_TASK();
    if (Base::RenderContext::SetFrameBuffersCount(frame_buffers_count))
    {
        ResetNativeSwapchain();
        return true;
    }
    return false;
}

const vk::Image& RenderContext::GetNativeFrameImage(uint32_t frame_buffer_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(frame_buffer_index, m_vk_frame_images.size());
    return m_vk_frame_images[frame_buffer_index];
}

const vk::Semaphore& RenderContext::GetNativeFrameImageAvailableSemaphore(uint32_t frame_buffer_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(frame_buffer_index, m_vk_frame_image_available_semaphores.size());
    return m_vk_frame_image_available_semaphores[frame_buffer_index];
}

const vk::Semaphore& RenderContext::GetNativeFrameImageAvailableSemaphore() const
{
    META_FUNCTION_TASK();
    return GetNativeFrameImageAvailableSemaphore(GetFrameBufferIndex());
}

uint32_t RenderContext::GetNextFrameBufferIndex()
{
    META_FUNCTION_TASK();
    uint32_t next_image_index = 0;
    const vk::Semaphore& vk_image_available_semaphore = m_vk_frame_semaphores_pool[Base::RenderContext::GetFrameBufferIndex()].get();
    if (const vk::Result image_acquire_result = m_vk_device.acquireNextImageKHR(GetNativeSwapchain(), std::numeric_limits<uint64_t>::max(), vk_image_available_semaphore, {}, &next_image_index);
        image_acquire_result != vk::Result::eSuccess &&
        image_acquire_result != vk::Result::eSuboptimalKHR)
        throw InvalidArgumentException<vk::Result>("RenderContext::GetNextFrameBufferIndex", "image_acquire_result", image_acquire_result, "failed to acquire next image");

    m_vk_frame_image_available_semaphores[next_image_index % m_vk_frame_image_available_semaphores.size()] = vk_image_available_semaphore;
    return next_image_index % Base::RenderContext::GetSettings().frame_buffers_count;
}

vk::SurfaceFormatKHR RenderContext::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) const
{
    META_FUNCTION_TASK();

    static constexpr vk::ColorSpaceKHR s_required_color_space  = vk::ColorSpaceKHR::eSrgbNonlinear;
    const vk::Format required_color_format = TypeConverter::PixelFormatToVulkan(GetSettings().color_format);

    const auto format_it = std::find_if(available_formats.begin(), available_formats.end(),
                                        [required_color_format](const vk::SurfaceFormatKHR& format)
                                        { return format.format == required_color_format &&
                                                 format.colorSpace == s_required_color_space; });
    if (format_it == available_formats.end())
        throw Rhi::IContext::IncompatibleException(fmt::format("{} surface format with {} color space is not available for window surface.",
                                                         magic_enum::enum_name(GetSettings().color_format), magic_enum::enum_name(s_required_color_space)));

    return *format_it;
}

vk::PresentModeKHR RenderContext::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) const
{
    META_FUNCTION_TASK();
    const std::vector<vk::PresentModeKHR> required_present_modes = GetSettings().vsync_enabled
        ? std::vector<vk::PresentModeKHR>{ vk::PresentModeKHR::eFifoRelaxed, vk::PresentModeKHR::eFifo }
        : std::vector<vk::PresentModeKHR>{ vk::PresentModeKHR::eMailbox,     vk::PresentModeKHR::eImmediate };

    std::optional<vk::PresentModeKHR> present_mode_opt;
    for (vk::PresentModeKHR required_present_mode : required_present_modes)
    {
        const auto present_mode_it = std::find_if(available_present_modes.begin(), available_present_modes.end(),
                                                  [required_present_mode](const vk::PresentModeKHR& present_mode)
                                                  { return present_mode == required_present_mode; });

        if (present_mode_it != available_present_modes.end())
        {
            present_mode_opt = required_present_mode;
            break;
        }
    }

    if (!present_mode_opt)
    {
        std::stringstream ss;
        for (vk::PresentModeKHR required_present_mode : required_present_modes)
        {
            ss << " " << magic_enum::enum_name(required_present_mode);
        }
        throw Rhi::IContext::IncompatibleException(fmt::format("None of required present modes ({}) is available for window surface.", ss.str()));
    }

    return *present_mode_opt;
}

vk::Extent2D RenderContext::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surface_caps) const
{
    META_FUNCTION_TASK();
    if (surface_caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surface_caps.currentExtent;

    const FrameSize& frame_size = GetSettings().frame_size;
    return vk::Extent2D(
        std::max(surface_caps.minImageExtent.width,  std::min(surface_caps.minImageExtent.width,  frame_size.GetWidth())),
        std::max(surface_caps.minImageExtent.height, std::min(surface_caps.minImageExtent.height, frame_size.GetHeight()))
    );
}

void RenderContext::InitializeNativeSwapchain()
{
    META_FUNCTION_TASK();

    if (const uint32_t present_queue_family_index = GetVulkanDevice().GetQueueFamilyReservation(Rhi::CommandListType::Render).GetFamilyIndex();
        !GetVulkanDevice().GetNativePhysicalDevice().getSurfaceSupportKHR(present_queue_family_index, GetNativeSurface()))
    {
        throw Rhi::IContext::IncompatibleException("Device does not support presentation to the window surface.");
    }

    const Device::SwapChainSupport swap_chain_support  = GetVulkanDevice().GetSwapChainSupportForSurface(GetNativeSurface());
    const vk::SurfaceFormatKHR       swap_surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
    const vk::PresentModeKHR         swap_present_mode   = ChooseSwapPresentMode(swap_chain_support.present_modes);
    const vk::Extent2D               swap_extent         = ChooseSwapExtent(swap_chain_support.capabilities);

    uint32_t image_count = std::max(swap_chain_support.capabilities.minImageCount, GetSettings().frame_buffers_count);
    if (swap_chain_support.capabilities.maxImageCount && image_count > swap_chain_support.capabilities.maxImageCount)
    {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    m_vk_unique_swapchain = m_vk_device.createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR(
            vk::SwapchainCreateFlagsKHR(),
            GetNativeSurface(),
            image_count,
            swap_surface_format.format,
            swap_surface_format.colorSpace,
            swap_extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive, 0, nullptr,
            swap_chain_support.capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            swap_present_mode,
            true
        )
    );
    m_vk_frame_images = m_vk_device.getSwapchainImagesKHR(GetNativeSwapchain());
    m_vk_frame_format = swap_surface_format.format;
    m_vk_frame_extent = swap_extent;

    if (m_vk_frame_images.size() != GetSettings().frame_buffers_count)
        InvalidateFrameBuffersCount(static_cast<uint32_t>(m_vk_frame_images.size()));

    // Create frame semaphores in pool
    const uint32_t frame_buffers_count = GetSettings().frame_buffers_count;
    m_vk_frame_semaphores_pool.resize(frame_buffers_count);
    for(vk::UniqueSemaphore& vk_unique_frame_semaphore : m_vk_frame_semaphores_pool)
    {
        if (vk_unique_frame_semaphore)
            continue;

        vk_unique_frame_semaphore = m_vk_device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
    }

    // Image available semaphores are assigned from frame semaphores in GetNextFrameBufferIndex
    m_vk_frame_image_available_semaphores.resize(frame_buffers_count);

    ResetNativeObjectNames();

    Data::Emitter<IRenderContextCallback>::Emit(&IRenderContextCallback::OnRenderContextSwapchainChanged, std::ref(*this));
}

void RenderContext::ReleaseNativeSwapchainResources()
{
    META_FUNCTION_TASK();
    WaitForGpu(WaitFor::RenderComplete);

    m_vk_frame_semaphores_pool.clear();
    m_vk_frame_image_available_semaphores.clear();
    m_vk_frame_images.clear();
    m_vk_unique_swapchain.reset();
}

void RenderContext::ResetNativeSwapchain()
{
    META_FUNCTION_TASK();
    ReleaseNativeSwapchainResources();
    InitializeNativeSwapchain();
    UpdateFrameBufferIndex();
}

void RenderContext::ResetNativeObjectNames() const
{
    META_FUNCTION_TASK();
    const std::string_view context_name = GetName();
    if (context_name.empty())
        return;

    SetVulkanObjectName(m_vk_device, m_vk_unique_surface.get(), context_name.data());

    uint32_t frame_index = 0u;
    for (const vk::UniqueSemaphore& vk_unique_frame_semaphore : m_vk_frame_semaphores_pool)
    {
        if (!vk_unique_frame_semaphore)
            continue;

        const std::string frame_semaphore_name = fmt::format("{} Frame {} ForImage Available", GetName(), frame_index);
        SetVulkanObjectName(m_vk_device, vk_unique_frame_semaphore.get(), frame_semaphore_name.c_str());
        frame_index++;
    }
}

} // namespace Methane::Graphics::Vulkan
