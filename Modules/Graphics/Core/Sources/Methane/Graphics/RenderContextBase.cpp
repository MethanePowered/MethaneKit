/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RenderContextBase.cpp
Base implementation of the render context interface.

******************************************************************************/

#include "RenderContextBase.h"
#include "DeviceBase.h"

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Graphics/CommandKit.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

RenderContext::Settings& RenderContext::Settings::SetFrameSize(FrameSize&& new_frame_size) noexcept
{
    META_FUNCTION_TASK();
    frame_size = std::move(new_frame_size);
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetColorFormat(PixelFormat new_color_format) noexcept
{
    META_FUNCTION_TASK();
    color_format = new_color_format;
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetDepthStencilFormat(PixelFormat new_ds_format) noexcept
{
    META_FUNCTION_TASK();
    depth_stencil_format = new_ds_format;
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetClearColor(Opt<Color4F>&& new_clear_color) noexcept
{
    META_FUNCTION_TASK();
    clear_color = std::move(new_clear_color);
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetClearDepthStencil(Opt<DepthStencil>&& new_clear_ds) noexcept
{
    META_FUNCTION_TASK();
    clear_depth_stencil = std::move(new_clear_ds);
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetFrameBuffersCount(uint32_t new_fb_count) noexcept
{
    META_FUNCTION_TASK();
    frame_buffers_count = new_fb_count;
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetVSyncEnabled(bool new_vsync_enabled) noexcept
{
    META_FUNCTION_TASK();
    vsync_enabled = new_vsync_enabled;
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetFullscreen(bool new_full_screen) noexcept
{
    META_FUNCTION_TASK();
    is_full_screen = new_full_screen;
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetOptionsMask(Options new_options_mask) noexcept
{
    META_FUNCTION_TASK();
    options_mask = new_options_mask;
    return *this;
}

RenderContext::Settings& RenderContext::Settings::SetUnsyncMaxFps(uint32_t new_unsync_max_fps) noexcept
{
    META_FUNCTION_TASK();
    unsync_max_fps = new_unsync_max_fps;
    return *this;
}

RenderContextBase::RenderContextBase(DeviceBase& device, UniquePtr<ResourceManager>&& resource_manager_ptr,
                                     tf::Executor& parallel_executor, const Settings& settings)
    : ContextBase(device, std::move(resource_manager_ptr), parallel_executor, Type::Render)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(m_settings.color_format, !IsSrgbColorSpace(m_settings.color_format),
                         "render context can not use color formats with sRGB gamma correction due to modern swap-chain flip model limitations");
}

void RenderContextBase::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    ContextBase::WaitForGpu(wait_for);

    switch (wait_for)
    {
    case WaitFor::RenderComplete: WaitForGpuRenderComplete(); break;
    case WaitFor::FramePresented: WaitForGpuFramePresented(); break;
    case WaitFor::ResourcesUploaded: break; // Handled in ContextBase::WaitForGpu
    default: META_UNEXPECTED_ARG(wait_for);
    }
}

void RenderContextBase::WaitForGpuRenderComplete()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::WaitForGpu::RenderComplete");

    OnGpuWaitStart(WaitFor::RenderComplete);
    GetRenderFence().FlushOnCpu();
    GetUploadCommandKit().GetFence().FlushOnCpu();
    OnGpuWaitComplete(WaitFor::RenderComplete);
}

void RenderContextBase::WaitForGpuFramePresented()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::WaitForGpu::FramePresented");

    OnGpuWaitStart(WaitFor::FramePresented);
    GetCurrentFrameFence().WaitOnCpu();
    OnGpuWaitComplete(WaitFor::FramePresented);
}

void RenderContextBase::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' RESIZE from {} to {}", GetName(), m_settings.frame_size, frame_size);

    m_settings.frame_size = frame_size;
}

void RenderContextBase::Present()
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' PRESENT frame {}", GetName(), m_frame_buffer_index);

    m_fps_counter.OnCpuFrameReadyToPresent();
}

void RenderContextBase::OnCpuPresentComplete(bool signal_frame_fence)
{
    META_FUNCTION_TASK();

    if (signal_frame_fence)
    {
        // Schedule a signal command in the queue for a currently finished frame
        GetCurrentFrameFence().Signal();
    }

    META_CPU_FRAME_DELIMITER(m_frame_buffer_index, m_frame_index);
    META_LOG("Render context '{}' PRESENT COMPLETE frame {}", GetName(), m_frame_buffer_index);

    m_fps_counter.OnCpuFramePresented();
}

Fence& RenderContextBase::GetCurrentFrameFence() const
{
    META_FUNCTION_TASK();
    return GetRenderCommandKit().GetFence(m_frame_buffer_index + 1);
}

Fence& RenderContextBase::GetRenderFence() const
{
    META_FUNCTION_TASK();
    return GetRenderCommandKit().GetFence(0U);
}

void RenderContextBase::ResetWithSettings(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' RESET with new settings", GetName());

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<DeviceBase> device_ptr = GetDeviceBase().GetPtr<DeviceBase>();
    m_settings = settings;

    Release();
    Initialize(*device_ptr, true);
}

void RenderContextBase::Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    ContextBase::Initialize(device, deferred_heap_allocation, false);

    m_frame_index = 0U;

    if (is_callback_emitted)
    {
        Emit(&IContextCallback::OnContextInitialized, *this);
    }
}

bool RenderContextBase::UploadResources()
{
    META_FUNCTION_TASK();
    if (!ContextBase::UploadResources())
        return false;

    // Render commands will wait for resources uploading completion in upload queue
    GetUploadCommandKit().GetFence().FlushOnGpu(GetRenderCommandKit().GetQueue());
    return true;
}

void RenderContextBase::OnGpuWaitStart(WaitFor wait_for)
{
    META_FUNCTION_TASK();

    if (wait_for == WaitFor::FramePresented)
    {
        m_fps_counter.OnGpuFramePresentWait();
    }
    ContextBase::OnGpuWaitStart(wait_for);
}

void RenderContextBase::OnGpuWaitComplete(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    if (wait_for == WaitFor::FramePresented)
    {
        m_fps_counter.OnGpuFramePresented();
        m_is_frame_buffer_in_use = false;
        PerformRequestedAction();
    }
    else
    {
        ContextBase::OnGpuWaitComplete(wait_for);
    }
}
    
void RenderContextBase::UpdateFrameBufferIndex()
{
    META_FUNCTION_TASK();
    m_frame_buffer_index = GetNextFrameBufferIndex();
    META_CHECK_ARG_LESS(m_frame_buffer_index, GetSettings().frame_buffers_count);
    m_frame_index++;
    m_is_frame_buffer_in_use = true;
}
    
uint32_t RenderContextBase::GetNextFrameBufferIndex()
{
    META_FUNCTION_TASK();
    return (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
}

bool RenderContextBase::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    if (m_settings.vsync_enabled == vsync_enabled)
        return false;

    m_settings.vsync_enabled = vsync_enabled;
    return true;
}

bool RenderContextBase::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    META_FUNCTION_TASK();
    frame_buffers_count = std::min(std::max(2U, frame_buffers_count), 10U);

    if (m_settings.frame_buffers_count == frame_buffers_count)
        return false;

    Settings new_settings = m_settings;
    new_settings.frame_buffers_count = frame_buffers_count;
    ResetWithSettings(new_settings);

    return true;
}

bool RenderContextBase::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (m_settings.is_full_screen == is_full_screen)
        return false;

    // No need to reset context for switching to full-screen
    // Application window state is kept in sync with context by the user code and handles window resizing
    m_settings.is_full_screen = is_full_screen;
    return true;
}

} // namespace Methane::Graphics
