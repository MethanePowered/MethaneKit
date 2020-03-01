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

FILE: Methane/Graphics/ContextBase.cpp
Base implementation of the context interface.

******************************************************************************/

#include "ContextBase.h"
#include "DeviceBase.h"
#include "CommandQueueBase.h"

#include <Methane/Graphics/BlitCommandList.h>
#include <Methane/Instrumentation.h>

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <cassert>

namespace Methane::Graphics
{

#ifdef COMMAND_EXECUTION_LOGGING
static std::string GetWaitForName(Context::WaitFor wait_for)
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
#endif

ContextBase::ContextBase(DeviceBase& device, Type type)
    : m_type(type)
    , m_sp_device(device.GetPtr())
    , m_resource_manager(*this)
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

    if (wait_for == WaitFor::ResourcesUploaded)
    {
        SCOPE_TIMER("ContextBase::WaitForGpu::ResourcesUploaded");
        assert(!!m_sp_upload_fence);
        OnGpuWaitStart(wait_for);
        m_sp_upload_fence->Flush();
        OnGpuWaitComplete(wait_for);
    }
}

void ContextBase::Reset(Device& device)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RESET context \"" + GetName() + "\" with device adapter \"" + device.GetAdapterName() + "\".");
#endif

    WaitForGpu(WaitFor::RenderComplete);
    Release();
    Initialize(static_cast<DeviceBase&>(device), false);
}

void ContextBase::Reset()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RESET context \"" + GetName() + "\"..");
#endif

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<DeviceBase> sp_device = m_sp_device;
    Release();
    Initialize(*sp_device, true);
}

void ContextBase::AddCallback(Callback& callback)
{
    ITT_FUNCTION_TASK();
    m_callbacks.push_back(callback);
}

void ContextBase::RemoveCallback(Callback& callback)
{
    ITT_FUNCTION_TASK();
    const auto callback_it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
        [&callback](const Ref<Callback>& callback_ref)
        {
            return std::addressof(callback_ref.get()) == std::addressof(callback);
        });
    assert(callback_it != m_callbacks.end());
    if (callback_it == m_callbacks.end())
        return;

    m_callbacks.erase(callback_it);
}

void ContextBase::OnGpuWaitComplete(WaitFor wait_for)
{
    ITT_FUNCTION_TASK();
    m_resource_manager.GetReleasePool().ReleaseResources();
}

void ContextBase::Release()
{
    ITT_FUNCTION_TASK();

    m_sp_device.reset();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("RELEASE context \"" + GetName() + "\"");
#endif

    m_sp_upload_cmd_queue.reset();
    m_sp_upload_cmd_list.reset();
    m_sp_upload_fence.reset();

    for (const Ref<Callback>& callback_ref : m_callbacks)
    {
        callback_ref.get().OnContextReleased();
    }

    m_resource_manager_init_settings.default_heap_sizes         = m_resource_manager.GetDescriptorHeapSizes(true, false);
    m_resource_manager_init_settings.shader_visible_heap_sizes  = m_resource_manager.GetDescriptorHeapSizes(true, true);

    m_resource_manager.Release();
}

void ContextBase::Initialize(DeviceBase& device, bool deferred_heap_allocation)
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("INITIALIZE context \"" + GetName() + "\"");
#endif

    m_sp_device = device.GetPtr();
    m_sp_upload_fence = Fence::Create(GetUploadCommandQueue());

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

    for (const Ref<Callback>& callback_ref : m_callbacks)
    {
        callback_ref.get().OnContextInitialized();
    }
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

BlitCommandList& ContextBase::GetUploadCommandList()
{
    ITT_FUNCTION_TASK();
    if (!m_sp_upload_cmd_list)
    {
        m_sp_upload_cmd_list = BlitCommandList::Create(GetUploadCommandQueue());
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
    
CommandQueueBase& ContextBase::GetUploadCommandQueueBase()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueBase&>(GetUploadCommandQueue());
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

void ContextBase::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();
    ObjectBase::SetName(name);
    GetDevice().SetName(name + " Device");

    if (m_sp_upload_fence)
        m_sp_upload_fence->SetName(name + " Upload Fence");
}

void ContextBase::UploadResources()
{
    ITT_FUNCTION_TASK();

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("UPLOAD resources for context \"" + GetName() + "\"");
#endif

    GetUploadCommandList().Commit();
    GetUploadCommandQueue().Execute({ GetUploadCommandList() });
    WaitForGpu(WaitFor::ResourcesUploaded);
}

void ContextBase::SetDevice(DeviceBase& device)
{
    m_sp_device = device.GetPtr();
}

} // namespace Methane::Graphics
