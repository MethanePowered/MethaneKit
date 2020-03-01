/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Instrumentation.h>

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <cassert>

namespace Methane::Graphics
{

RenderContextBase::RenderContextBase(DeviceBase& device, const Settings& settings)
    : ContextBase(device, Type::Render)
    , m_settings(settings)
    , m_frame_buffer_index(0)
{
    ITT_FUNCTION_TASK();
}

void RenderContextBase::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

    ContextBase::WaitForGpu(wait_for);

    switch (wait_for)
    {
    case WaitFor::RenderComplete:
    {
        SCOPE_TIMER("RenderContextDX::WaitForGpu::RenderComplete");
        OnGpuWaitStart(wait_for);
        GetRenderFence().Flush();
        OnGpuWaitComplete(wait_for);
    } break;

    case WaitFor::FramePresented:
    {
        SCOPE_TIMER("RenderContextDX::WaitForGpu::FramePresented");
        OnGpuWaitStart(wait_for);
        GetCurrentFrameFence().Wait();
        OnGpuWaitComplete(wait_for);
    } break;

    case WaitFor::ResourcesUploaded: break; // Handled in ContextBase::WaitForGpu
    }
}

void RenderContextBase::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RESIZE context \"" + GetName() + "\" from " + static_cast<std::string>(m_settings.frame_size) + " to " + static_cast<std::string>(frame_size));
#endif

    m_settings.frame_size = frame_size;
}

void RenderContextBase::Present()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("PRESENT frame " + std::to_string(m_frame_buffer_index) + " in context \"" + GetName() + "\"");
#endif

    m_fps_counter.OnCpuFrameReadyToPresent();
}

void RenderContextBase::OnCpuPresentComplete(bool signal_frame_fence)
{
    ITT_FUNCTION_TASK();

    if (signal_frame_fence)
    {
        // Schedule a signal command in the queue for a currently finished frame
        GetCurrentFrameFence().Signal();
    }

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("PRESENT COMPLETE for context \"" + GetName() + "\"");
#endif

    m_fps_counter.OnCpuFramePresented();
}

Fence& RenderContextBase::GetCurrentFrameFence() const
{
    ITT_FUNCTION_TASK();
    const UniquePtr<Fence>& sp_current_fence = GetCurrentFrameFencePtr();
    assert(!!sp_current_fence);
    return *sp_current_fence;
}

Fence& RenderContextBase::GetRenderFence() const
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_render_fence);
    return *m_sp_render_fence;
}

void RenderContextBase::ResetWithSettings(const Settings& settings)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RESET context \"" + GetName() + "\" with new settings.");
#endif

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<DeviceBase> sp_device = GetDeviceBase().GetPtr();
    m_settings = settings;

    Release();
    Initialize(*sp_device, true);
}

void RenderContextBase::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

    ContextBase::Initialize(device, deferred_heap_allocation);

    m_frame_fences.clear();
    for (uint32_t frame_index = 0; frame_index < m_settings.frame_buffers_count; ++frame_index)
    {
        m_frame_fences.emplace_back(Fence::Create(GetRenderCommandQueue()));
    }

    m_sp_render_fence = Fence::Create(GetRenderCommandQueue());
}

void RenderContextBase::Release()
{
    ITT_FUNCTION_TASK();

    m_sp_render_fence.reset();
    m_frame_fences.clear();
    m_sp_render_cmd_queue.reset();

    ContextBase::Release();
}

void RenderContextBase::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ContextBase::SetName(name);

    for (uint32_t frame_index = 0; frame_index < m_frame_fences.size(); ++frame_index)
    {
        const UniquePtr<Fence>& sp_frame_fence = m_frame_fences[frame_index];
        assert(!!sp_frame_fence);
        sp_frame_fence->SetName(name + " Frame " + std::to_string(frame_index) + " Fence");
    }

    if (m_sp_render_fence)
        m_sp_render_fence->SetName(name + " Render Fence");
}
    
void RenderContextBase::OnGpuWaitStart(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    if (wait_for == WaitFor::FramePresented)
    {
        m_fps_counter.OnGpuFramePresentWait();
    }
    ContextBase::OnGpuWaitStart(wait_for);
}

void RenderContextBase::OnGpuWaitComplete(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    if (wait_for == WaitFor::FramePresented)
    {
        m_fps_counter.OnGpuFramePresented();
    }
    ContextBase::OnGpuWaitComplete(wait_for);
}
    
void RenderContextBase::UpdateFrameBufferIndex()
{
    m_frame_buffer_index = GetNextFrameBufferIndex();
}
    
uint32_t RenderContextBase::GetNextFrameBufferIndex()
{
    return (m_frame_buffer_index + 1) % m_settings.frame_buffers_count;
}

CommandQueue& RenderContextBase::GetRenderCommandQueue()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_render_cmd_queue)
    {
        m_sp_render_cmd_queue = CommandQueue::Create(*this);
        m_sp_render_cmd_queue->SetName("Render Command Queue");
    }
    return *m_sp_render_cmd_queue;
}

bool RenderContextBase::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    if (m_settings.vsync_enabled == vsync_enabled)
        return false;

    m_settings.vsync_enabled = vsync_enabled;
    return true;
}

bool RenderContextBase::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    ITT_FUNCTION_TASK();
    frame_buffers_count = std::min(std::max(2u, frame_buffers_count), 10u);

    if (m_settings.frame_buffers_count == frame_buffers_count)
        return false;

    Settings new_settings = m_settings;
    new_settings.frame_buffers_count = frame_buffers_count;
    ResetWithSettings(new_settings);

    return true;
}

bool RenderContextBase::SetFullScreen(bool is_full_screen)
{
    ITT_FUNCTION_TASK();
    if (m_settings.is_full_screen == is_full_screen)
        return false;

    // No need to reset context for switching to full-screen
    // Application window state is kept in sync with context by the user code and handles window resizing
    m_settings.is_full_screen = is_full_screen;
    return true;
}

} // namespace Methane::Graphics
