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

FILE: Methane/Graphics/Base/RenderContext.cpp
Base implementation of the render context interface.

******************************************************************************/

#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Graphics/Base/Device.h>

#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

RenderContext::RenderContext(Device& device, UniquePtr<Rhi::IDescriptorManager>&& descriptor_manager_ptr,
                                     tf::Executor& parallel_executor, const Settings& settings)
    : Context(device, std::move(descriptor_manager_ptr), parallel_executor, Type::Render)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_DESCR(m_settings.color_format, !IsSrgbColorSpace(m_settings.color_format),
                         "render context can not use color formats with sRGB gamma correction due to modern swap-chain flip model limitations");
}

void RenderContext::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    Context::WaitForGpu(wait_for);

    switch (wait_for)
    {
    case WaitFor::RenderComplete: WaitForGpuRenderComplete(); break;
    case WaitFor::FramePresented: WaitForGpuFramePresented(); break;
    case WaitFor::ResourcesUploaded: break; // Handled in Context::WaitForGpu
    default: META_UNEXPECTED_ARG(wait_for);
    }
}

void RenderContext::WaitForGpuRenderComplete()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::WaitForGpu::RenderComplete");

    OnGpuWaitStart(WaitFor::RenderComplete);
    GetRenderFence().FlushOnCpu();
    GetUploadCommandKit().GetFence().FlushOnCpu();
    OnGpuWaitComplete(WaitFor::RenderComplete);
}

void RenderContext::WaitForGpuFramePresented()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::WaitForGpu::FramePresented");

    OnGpuWaitStart(WaitFor::FramePresented);
    GetCurrentFrameFence().WaitOnCpu();
    OnGpuWaitComplete(WaitFor::FramePresented);
}

void RenderContext::Resize(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' RESIZE from {} to {}", GetName(), m_settings.frame_size, frame_size);

    m_settings.frame_size = frame_size;
}

void RenderContext::Present()
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' PRESENT frame {}", GetName(), m_frame_buffer_index);

    m_fps_counter.OnCpuFrameReadyToPresent();
}

void RenderContext::OnCpuPresentComplete(bool signal_frame_fence)
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

Rhi::IFence& RenderContext::GetCurrentFrameFence() const
{
    META_FUNCTION_TASK();
    return GetRenderCommandKit().GetFence(m_frame_buffer_index + 1);
}

Rhi::IFence& RenderContext::GetRenderFence() const
{
    META_FUNCTION_TASK();
    return GetRenderCommandKit().GetFence(0U);
}

void RenderContext::ResetWithSettings(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_LOG("Render context '{}' RESET with new settings", GetName());

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<Device> device_ptr = GetBaseDevice().GetPtr<Device>();
    m_settings = settings;

    Release();
    Initialize(*device_ptr, true);
}

void RenderContext::Initialize(Device& device, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    Context::Initialize(device, false);

    m_frame_index = 0U;

    if (is_callback_emitted)
    {
        Data::Emitter<Rhi::IContextCallback>::Emit(&Rhi::IContextCallback::OnContextInitialized, *this);
    }
}

bool RenderContext::UploadResources() const
{
    META_FUNCTION_TASK();
    if (!Context::UploadResources())
        return false;

    // Render commands will wait for resources uploading completion in upload queue
    GetUploadCommandKit().GetFence().FlushOnGpu(GetRenderCommandKit().GetQueue());
    return true;
}

void RenderContext::OnGpuWaitStart(WaitFor wait_for)
{
    META_FUNCTION_TASK();

    if (wait_for == WaitFor::FramePresented)
    {
        m_fps_counter.OnGpuFramePresentWait();
    }
    Context::OnGpuWaitStart(wait_for);
}

void RenderContext::OnGpuWaitComplete(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    if (wait_for == WaitFor::FramePresented)
    {
        m_fps_counter.OnGpuFramePresented();
        PerformRequestedAction();
    }
    else
    {
        Context::OnGpuWaitComplete(wait_for);
    }
}
    
void RenderContext::UpdateFrameBufferIndex()
{
    META_FUNCTION_TASK();
    m_frame_buffer_index = GetNextFrameBufferIndex();
    META_CHECK_ARG_LESS(m_frame_buffer_index, GetSettings().frame_buffers_count);
    m_frame_index++;
}

void RenderContext::InvalidateFrameBuffersCount(uint32_t frame_buffers_count)
{
    META_FUNCTION_TASK();
    // We just change count in settings assuming that this method is called only inside RenderContextXX::Initialize()s function
    m_settings.frame_buffers_count = frame_buffers_count;
}

void RenderContext::InvalidateFrameBufferIndex(uint32_t frame_buffer_index)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(frame_buffer_index, m_settings.frame_buffers_count);
    m_frame_buffer_index = frame_buffer_index;
}

uint32_t RenderContext::GetNextFrameBufferIndex()
{
    META_FUNCTION_TASK();
    return (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
}

bool RenderContext::SetVSyncEnabled(bool vsync_enabled)
{
    META_FUNCTION_TASK();
    if (m_settings.vsync_enabled == vsync_enabled)
        return false;

    m_settings.vsync_enabled = vsync_enabled;
    return true;
}

bool RenderContext::SetFrameBuffersCount(uint32_t frame_buffers_count)
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

bool RenderContext::SetFullScreen(bool is_full_screen)
{
    META_FUNCTION_TASK();
    if (m_settings.is_full_screen == is_full_screen)
        return false;

    // No need to reset context for switching to full-screen
    // Application window state is kept in sync with context by the user code and handles window resizing
    m_settings.is_full_screen = is_full_screen;
    return true;
}

} // namespace Methane::Graphics::Base
