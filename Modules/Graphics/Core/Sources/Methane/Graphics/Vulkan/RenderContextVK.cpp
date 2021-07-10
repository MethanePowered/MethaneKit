/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/RenderContextVK.mm
Vulkan implementation of the render context interface.

******************************************************************************/

#include "RenderContextVK.h"
#include "DeviceVK.h"
#include "RenderStateVK.h"
#include "CommandQueueVK.h"
#include "PlatformVK.h"
#include "TypesVK.h"

#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

Ptr<RenderContext> RenderContext::Create(const Platform::AppEnvironment& env, Device& device, tf::Executor& parallel_executor, const RenderContext::Settings& settings)
{
    META_FUNCTION_TASK();
    auto& device_base = static_cast<DeviceBase&>(device);
    const auto render_context_ptr = std::make_shared<RenderContextVK>(env, device_base, parallel_executor, settings);
    render_context_ptr->Initialize(device_base, true);
    return render_context_ptr;
}

RenderContextVK::RenderContextVK(const Platform::AppEnvironment& app_env, DeviceBase& device, tf::Executor& parallel_executor, const RenderContext::Settings& settings)
    : ContextVK<RenderContextBase>(device, parallel_executor, settings)
    , m_vk_surface(PlatformVK::CreateVulkanSurfaceForWindow(static_cast<SystemVK&>(System::Get()).GetNativeInstance(), app_env))
{
    META_FUNCTION_TASK();
}

RenderContextVK::~RenderContextVK()
{
    META_FUNCTION_TASK();
    Release();

    static_cast<SystemVK&>(System::Get()).GetNativeInstance().destroy(m_vk_surface);
}

void RenderContextVK::Release()
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Release();

    const vk::Device& vk_device = GetDeviceVK().GetNativeDevice();
    for(vk::Semaphore& vk_semaphore : m_vk_frame_semaphores_pool)
        vk_device.destroy(vk_semaphore);

    m_vk_frame_semaphores_pool.clear();
    m_vk_frame_image_available_semaphores.clear();
    m_vk_frame_images.clear();

    vk_device.destroy(m_vk_swapchain);
}

void RenderContextVK::Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Initialize(device, deferred_heap_allocation, is_callback_emitted);

    const uint32_t present_queue_family_index = GetDeviceVK().GetQueueFamilyReservation(CommandList::Type::Render).GetFamilyIndex();
    if (!GetDeviceVK().GetNativePhysicalDevice().getSurfaceSupportKHR(present_queue_family_index, m_vk_surface))
        throw Context::IncompatibleException("Device does not support presentation to the window surface.");

    const DeviceVK::SwapChainSupport swap_chain_support  = GetDeviceVK().GetSwapChainSupportForSurface(m_vk_surface);
    const vk::SurfaceFormatKHR       swap_surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
    const vk::PresentModeKHR         swap_present_mode   = ChooseSwapPresentMode(swap_chain_support.present_modes);
    const vk::Extent2D               swap_extent         = ChooseSwapExtent(swap_chain_support.capabilities);

    uint32_t image_count = std::max(swap_chain_support.capabilities.minImageCount, GetSettings().frame_buffers_count);
    if (swap_chain_support.capabilities.maxImageCount && image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    const vk::Device& vk_device = GetDeviceVK().GetNativeDevice();
    m_vk_swapchain = vk_device.createSwapchainKHR(
        vk::SwapchainCreateInfoKHR(
            vk::SwapchainCreateFlagsKHR(),
            m_vk_surface,
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
            true,
            nullptr
        )
    );

    m_vk_frame_images = vk_device.getSwapchainImagesKHR(m_vk_swapchain);
    m_vk_frame_format = swap_surface_format.format;
    m_vk_frame_extent = swap_extent;

    m_vk_frame_semaphores_pool.resize(GetSettings().frame_buffers_count);
    for(vk::Semaphore& vk_frame_semaphore : m_vk_frame_semaphores_pool)
    {
        vk_frame_semaphore = vk_device.createSemaphore(vk::SemaphoreCreateInfo());
    }
    // Image available semaphores are assigned from frame semaphores in GetNextFrameBufferIndex
    m_vk_frame_image_available_semaphores.resize(GetSettings().frame_buffers_count);

    UpdateFrameBufferIndex();
}

bool RenderContextVK::ReadyToRender() const
{
    META_FUNCTION_TASK();
    return true;
}

void RenderContextVK::WaitForGpu(Context::WaitFor wait_for)
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::WaitForGpu(wait_for);
}

void RenderContextVK::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Resize(frame_size);
    UpdateFrameBufferIndex();
}

void RenderContextVK::Present()
{
    META_FUNCTION_TASK();
    ContextVK<RenderContextBase>::Present();
    // ...
    ContextVK<RenderContextBase>::OnCpuPresentComplete();
    UpdateFrameBufferIndex();
}

bool RenderContextVK::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    return RenderContextBase::SetVSyncEnabled(vsync_enabled);
}

bool RenderContextVK::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    META_FUNCTION_TASK();
    return RenderContextBase::SetFrameBuffersCount(frame_buffers_count);
}

float RenderContextVK::GetContentScalingFactor() const
{
    META_FUNCTION_TASK();
    return 1.F;
}

uint32_t RenderContextVK::GetFontResolutionDpi() const
{
    META_FUNCTION_TASK();
    return 96U;
}

const vk::Image& RenderContextVK::GetNativeFrameImage(uint32_t frame_buffer_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(frame_buffer_index, m_vk_frame_images.size());
    return m_vk_frame_images[frame_buffer_index];
}

const vk::Semaphore& RenderContextVK::GetNativeFrameImageAvailableSemaphore(uint32_t frame_buffer_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(frame_buffer_index, m_vk_frame_image_available_semaphores.size());
    return m_vk_frame_image_available_semaphores[frame_buffer_index];
}

const vk::Semaphore& RenderContextVK::GetNativeFrameImageAvailableSemaphore() const
{
    META_FUNCTION_TASK();
    return GetNativeFrameImageAvailableSemaphore(GetFrameBufferIndex());
}

uint32_t RenderContextVK::GetNextFrameBufferIndex()
{
    META_FUNCTION_TASK();
    uint32_t next_frame_index = 0;
    const vk::Semaphore& vk_image_available_semaphore = m_vk_frame_semaphores_pool[GetFrameIndex() % m_vk_frame_semaphores_pool.size()];
    vk::Result image_acquire_result = GetDeviceVK().GetNativeDevice().acquireNextImageKHR(m_vk_swapchain, std::numeric_limits<uint64_t>::max(), vk_image_available_semaphore, {}, &next_frame_index);
    META_CHECK_ARG_EQUAL_DESCR(image_acquire_result, vk::Result::eSuccess, "Failed to acquire next image");
    m_vk_frame_image_available_semaphores[next_frame_index] = vk_image_available_semaphore;
    return next_frame_index;
}

vk::SurfaceFormatKHR RenderContextVK::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) const
{
    META_FUNCTION_TASK();

    static const vk::ColorSpaceKHR s_required_color_space  = vk::ColorSpaceKHR::eSrgbNonlinear;
    const vk::Format required_color_format = TypeConverterVK::PixelFormatToVulkan(GetSettings().color_format);

    const auto format_it = std::find_if(available_formats.begin(), available_formats.end(),
                                        [required_color_format](const vk::SurfaceFormatKHR& format)
                                        { return format.format == required_color_format &&
                                                 format.colorSpace == s_required_color_space; });
    if (format_it == available_formats.end())
        throw Context::IncompatibleException(fmt::format("{} surface format with {} color space is not available for window surface.",
                                                         magic_enum::enum_name(required_color_format), magic_enum::enum_name(s_required_color_space)));

    return *format_it;
}

vk::PresentModeKHR RenderContextVK::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) const
{
    META_FUNCTION_TASK();
    const vk::PresentModeKHR required_present_mode = GetSettings().vsync_enabled
                                                   ? vk::PresentModeKHR::eFifo
                                                   : vk::PresentModeKHR::eMailbox;

    const auto present_mode_it = std::find_if(available_present_modes.begin(), available_present_modes.end(),
                                              [required_present_mode](const vk::PresentModeKHR& present_mode)
                                              { return present_mode == required_present_mode; });

    if (present_mode_it == available_present_modes.end())
        throw Context::IncompatibleException(fmt::format("{} present mode is not available for window surface.", magic_enum::enum_name(required_present_mode)));

    return *present_mode_it;
}

vk::Extent2D RenderContextVK::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surface_caps) const
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

} // namespace Methane::Graphics
