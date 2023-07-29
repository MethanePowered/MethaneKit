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

FILE: Methane/Graphics/Vulkan/RenderContext.hh
Vulkan implementation of the render context interface.

******************************************************************************/

#pragma once

#include "Context.hpp"

#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Data/Emitter.hpp>

#include <deque>
#include <vulkan/vulkan.hpp>

#ifdef __APPLE__
#ifdef __OBJC__
#import <Methane/Platform/MacOS/AppViewMetal.hh>
#else
using AppViewMetal = void;
#endif
#endif

namespace Methane::Graphics::Vulkan
{

class RenderContext;

struct IRenderContextCallback
{
    virtual void OnRenderContextSwapchainChanged(RenderContext& context) = 0;

    virtual ~IRenderContextCallback() = default;
};

class RenderContext final // NOSONAR - this class requires destructor
    : public Context<Base::RenderContext>
    , public Data::Emitter<IRenderContextCallback>
{
public:
    RenderContext(const Methane::Platform::AppEnvironment& app_env, Device& device,
                  tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings);
    ~RenderContext() override;

    // IContext interface
    void WaitForGpu(WaitFor wait_for) override;

    // IRenderContext interface
    [[nodiscard]] Ptr<Rhi::ITexture> CreateTexture(const Rhi::TextureSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IRenderState> CreateRenderState(const Rhi::RenderStateSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IRenderPattern> CreateRenderPattern(const Rhi::RenderPatternSettings& settings) override;
    bool     ReadyToRender() const override;
    void     Resize(const FrameSize& frame_size) override;
    void     Present() override;
    bool     SetVSyncEnabled(bool vsync_enabled) override;
    bool     SetFrameBuffersCount(uint32_t frame_buffers_count) override;
    Methane::Platform::AppView GetAppView() const override { return { }; }

    // Base::Context overrides
    void Initialize(Base::Device& device, bool is_callback_emitted = true) override;
    void Release() override;

    // Base::Object overrides
    bool SetName(std::string_view name) override;

    const vk::SurfaceKHR&   GetNativeSurface() const noexcept     { return m_vk_unique_surface.get(); }
    const vk::SwapchainKHR& GetNativeSwapchain() const noexcept   { return m_vk_unique_swapchain.get(); }
    const vk::Extent2D&     GetNativeFrameExtent() const noexcept { return m_vk_frame_extent; }
    vk::Format              GetNativeFrameFormat() const noexcept { return m_vk_frame_format; }
    const vk::Image&        GetNativeFrameImage(uint32_t frame_buffer_index) const;
    const vk::Semaphore&    GetNativeFrameImageAvailableSemaphore(uint32_t frame_buffer_index) const;
    const vk::Semaphore&    GetNativeFrameImageAvailableSemaphore() const;

    void DeferredRelease(vk::UniquePipeline&& pipeline) const { m_vk_deferred_release_pipelines.emplace_back(std::move(pipeline)); }

protected:
    // Base::RenderContext overrides
    uint32_t GetNextFrameBufferIndex() override;

private:
    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) const;
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) const;
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surface_caps) const;
    void InitializeNativeSwapchain();
    void ReleaseNativeSwapchainResources();
    void ResetNativeSwapchain();
    void ResetNativeObjectNames() const;

    const Methane::Platform::AppEnvironment m_app_env;
    const vk::Device                        m_vk_device;
#ifdef __APPLE__
    // MacOS metal app view with swap-chain implementation to work via MoltenVK
    AppViewMetal* m_metal_view;
#endif
    vk::UniqueSurfaceKHR                   m_vk_unique_surface;
    vk::UniqueSwapchainKHR                 m_vk_unique_swapchain;
    vk::Format                             m_vk_frame_format;
    vk::Extent2D                           m_vk_frame_extent;
    std::vector<vk::Image>                 m_vk_frame_images;
    std::vector<vk::UniqueSemaphore>       m_vk_frame_semaphores_pool;
    std::vector<vk::Semaphore>             m_vk_frame_image_available_semaphores;
    mutable std::deque<vk::UniquePipeline> m_vk_deferred_release_pipelines;
};

} // namespace Methane::Graphics::Vulkan
