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

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

RenderContextBase::RenderContextBase(DeviceBase& device, tf::Executor& parallel_executor, const Settings& settings)
    : ContextBase(device, parallel_executor, Type::Render)
    , m_settings(settings)
    , m_frame_buffer_index(0)
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
    default: META_UNEXPECTED_ENUM_ARG(wait_for);
    }
}

void RenderContextBase::WaitForGpuRenderComplete()
{
    META_FUNCTION_TASK();
    META_SCOPE_TIMER("RenderContextDX::WaitForGpu::RenderComplete");

    OnGpuWaitStart(WaitFor::RenderComplete);
    GetRenderFence().FlushOnCpu();
    GetUploadFence().FlushOnCpu();
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
    META_LOG("RESIZE context \"" + GetName() + "\" from " + static_cast<std::string>(m_settings.frame_size) + " to " + static_cast<std::string>(frame_size));

    m_settings.frame_size = frame_size;
}

void RenderContextBase::Present()
{
    META_FUNCTION_TASK();
    META_LOG("PRESENT frame " + std::to_string(m_frame_buffer_index) + " in context \"" + GetName() + "\"");

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
    META_LOG("PRESENT COMPLETE for context \"" + GetName() + "\"");

    m_fps_counter.OnCpuFramePresented();
}

Fence& RenderContextBase::GetCurrentFrameFence() const
{
    META_FUNCTION_TASK();
    const Ptr<Fence>& current_fence_ptr = GetCurrentFrameFencePtr();
    assert(!!current_fence_ptr);
    return *current_fence_ptr;
}

Fence& RenderContextBase::GetRenderFence() const
{
    META_FUNCTION_TASK();
    assert(!!m_render_fence_ptr);
    return *m_render_fence_ptr;
}

void RenderContextBase::ResetWithSettings(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_LOG("RESET context \"" + GetName() + "\" with new settings.");

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<DeviceBase> device_ptr = GetDeviceBase().GetDevicePtr();
    m_settings = settings;

    Release();
    Initialize(*device_ptr, true);
}

void RenderContextBase::Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted)
{
    META_FUNCTION_TASK();

    ContextBase::Initialize(device, deferred_heap_allocation, false);

    m_frame_fences.clear();
    for (uint32_t frame_index = 0; frame_index < m_settings.frame_buffers_count; ++frame_index)
    {
        m_frame_fences.emplace_back(Fence::Create(GetRenderCommandQueue()));
    }

    m_render_fence_ptr = Fence::Create(GetRenderCommandQueue());
    m_frame_index = 0U;

    if (is_callback_emitted)
    {
        Emit(&IContextCallback::OnContextInitialized, *this);
    }
}

void RenderContextBase::Release()
{
    META_FUNCTION_TASK();

    m_render_fence_ptr.reset();
    m_frame_fences.clear();
    m_render_cmd_queue_ptr.reset();

    ContextBase::Release();
}

void RenderContextBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ContextBase::SetName(name);

    for (uint32_t frame_index = 0; frame_index < m_frame_fences.size(); ++frame_index)
    {
        const Ptr<Fence>& frame_fence_ptr = m_frame_fences[frame_index];
        assert(!!frame_fence_ptr);
        frame_fence_ptr->SetName(name + " Frame " + std::to_string(frame_index) + " Fence");
    }

    if (m_render_fence_ptr)
        m_render_fence_ptr->SetName(name + " Render Fence");
}

bool RenderContextBase::UploadResources()
{
    META_FUNCTION_TASK();
    if (!ContextBase::UploadResources())
        return false;

    // Render commands will wait for resources uploading completion in upload queue
    GetUploadFence().FlushOnGpu(GetRenderCommandQueue());
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
    m_frame_buffer_index = GetNextFrameBufferIndex();
    m_frame_index++;
    m_is_frame_buffer_in_use = true;
}
    
uint32_t RenderContextBase::GetNextFrameBufferIndex()
{
    return (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
}

CommandQueue& RenderContextBase::GetRenderCommandQueue()
{
    META_FUNCTION_TASK();
    if (!m_render_cmd_queue_ptr)
    {
        static const std::string s_command_queue_name = "Render Command Queue";
        m_render_cmd_queue_ptr = CommandQueue::Create(*this, CommandList::Type::Render);
        m_render_cmd_queue_ptr->SetName(s_command_queue_name);
    }
    return *m_render_cmd_queue_ptr;
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
