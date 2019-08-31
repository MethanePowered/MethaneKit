/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ContextBase.cpp
Base implementation of the context interface.

******************************************************************************/

#include "ContextBase.h"
#include "DeviceBase.h"
#include "Instrumentation.h"

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <cassert>

using namespace Methane;
using namespace Methane::Graphics;

std::string GetWaitForName(Context::WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    switch (wait_for)
    {
    case Context::WaitFor::RenderComplete:      return "WAIT for Render Complete";
    case Context::WaitFor::FramePresented:      return "WAIT for Frame Present";
    case Context::WaitFor::ResourcesUploaded:   return "WAIT for Resources Upload";
    }
    return "";
}

ContextBase::ContextBase(const Data::Provider& data_provider, DeviceBase& device, const Settings& settings)
    : m_data_provider(data_provider)
    , m_sp_device(device.GetPtr())
    , m_settings(settings)
    , m_resource_manager(*this)
    , m_frame_buffer_index(0)
{
    ITT_FUNCTION_TASK();
}

void ContextBase::CompleteInitialization()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("Complete initialization of context \"" + GetName() + "\"");
#endif

    m_resource_manager.CompleteInitialization();
    UploadResources();
}

void ContextBase::WaitForGpu(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput(GetWaitForName(wait_for) + " in context \"" + GetName() + "\"");
#endif

    m_resource_manager.GetReleasePool().ReleaseResources();
}

void ContextBase::Resize(const FrameSize& frame_size)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RESIZE context \"" + GetName() + "\" from " + static_cast<std::string>(m_settings.frame_size) + " to " + static_cast<std::string>(frame_size));
#endif

    m_settings.frame_size = frame_size;
}

void ContextBase::Reset(Device& device)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RESET context \"" + GetName() + "\"");
#endif

    WaitForGpu(WaitFor::RenderComplete);
    Release();
    Initialize(device, false);
}

void ContextBase::Present()
{
#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("PRESENT frame " + std::to_string(m_frame_buffer_index) + " in context \"" + GetName() + "\"");
#endif
}

void ContextBase::AddCallback(Callback& callback)
{
    m_callbacks.push_back(callback);
}

void ContextBase::RemoveCallback(Callback& callback)
{
    const auto callback_it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
                                          [&callback](const Callback::Ref& callback_ref)
                                          { return std::addressof(callback_ref.get()) == std::addressof(callback); });
    assert(callback_it != m_callbacks.end());
    if (callback_it == m_callbacks.end())
        return;
    
    m_callbacks.erase(callback_it);
}

void ContextBase::OnPresentComplete()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("PRESENT COMPLETE for context \"" + GetName() + "\"");
#endif

    m_fps_counter.OnFramePresented();
}

void ContextBase::Release()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RELEASE context \"" + GetName() + "\"");
#endif

    m_sp_render_cmd_queue.reset();
    m_sp_upload_cmd_queue.reset();
    m_sp_upload_cmd_list.reset();

    for (const Callback::Ref& callback_ref : m_callbacks)
    {
        callback_ref.get().OnContextReleased();
    }

    m_resource_manager_init_settings.default_heap_sizes         = m_resource_manager.GetDescriptorHeapSizes(true, false);
    m_resource_manager_init_settings.shader_visible_heap_sizes  = m_resource_manager.GetDescriptorHeapSizes(true, true);
    m_resource_manager.Release();
}

void ContextBase::Initialize(Device& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("INITIALIZE context \"" + GetName() + "\"");
#endif

    m_sp_device = static_cast<DeviceBase&>(device).GetPtr();
    
    const std::string& context_name = GetName();
    if (!context_name.empty())
    {
        m_sp_device->SetName(context_name + " Device");
    }

    m_resource_manager_init_settings.deferred_heap_allocation = deferred_heap_allocation;
    if (deferred_heap_allocation)
    {
        m_resource_manager_init_settings.default_heap_sizes        = {};
        m_resource_manager_init_settings.shader_visible_heap_sizes = {};
    }
    m_resource_manager.Initialize(m_resource_manager_init_settings);

    for (const Callback::Ref& callback_ref : m_callbacks)
    {
        callback_ref.get().OnContextInitialized();
    }
}

CommandQueue& ContextBase::GetRenderCommandQueue()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_render_cmd_queue)
    {
        m_sp_render_cmd_queue = CommandQueue::Create(*this);
        m_sp_render_cmd_queue->SetName("Render Command Queue");
    }
    return *m_sp_render_cmd_queue;
}

CommandQueue& ContextBase::GetUploadCommandQueue()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_upload_cmd_queue)
    {
        m_sp_upload_cmd_queue = CommandQueue::Create(*this);
        m_sp_upload_cmd_queue->SetName("Upload Command Queue");
    }
    return *m_sp_upload_cmd_queue;
}

RenderCommandList& ContextBase::GetUploadCommandList()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_upload_cmd_list)
    {
        RenderPass::Ptr sp_empty_pass = RenderPass::Create(*this, RenderPass::Settings());
        m_sp_upload_cmd_list = RenderCommandList::Create(GetUploadCommandQueue(), *sp_empty_pass);
        m_sp_upload_cmd_list->SetName("Upload Command List");
    }
    return *m_sp_upload_cmd_list;
}

Device& ContextBase::GetDevice()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_device);
    return *m_sp_device;
}

DeviceBase& ContextBase::GetDeviceBase()
{
    ITT_FUNCTION_TASK();
    return static_cast<DeviceBase&>(GetDevice());
}

const DeviceBase& ContextBase::GetDeviceBase() const
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_device);
    return static_cast<const DeviceBase&>(*m_sp_device);
}

bool ContextBase::SetVSyncEnabled(bool vsync_enabled)
{
    ITT_FUNCTION_TASK();
    if (m_settings.vsync_enabled == vsync_enabled)
        return false;

    m_settings.vsync_enabled = vsync_enabled;
    return true;
}

bool ContextBase::SetFrameBuffersCount(uint32_t frame_buffers_count)
{
    ITT_FUNCTION_TASK();
    frame_buffers_count = std::min(std::max(2u, frame_buffers_count), 10u);

    if (m_settings.frame_buffers_count == frame_buffers_count)
        return false;

    WaitForGpu(WaitFor::RenderComplete);

    DeviceBase::Ptr sp_device = m_sp_device;
    m_settings.frame_buffers_count = frame_buffers_count;

    Release();
    Initialize(*sp_device, true);

    return true;
}

void ContextBase::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    ObjectBase::SetName(name);
    GetDevice().SetName(name + " Device");
}

void ContextBase::UploadResources()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("UPLOAD resources for context \"" + GetName() + "\"");
#endif

    GetUploadCommandList().Commit(false);
    GetUploadCommandQueue().Execute({ GetUploadCommandList() });
    WaitForGpu(WaitFor::ResourcesUploaded);
}
