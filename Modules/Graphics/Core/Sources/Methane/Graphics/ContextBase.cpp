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

#include <cassert>

namespace Methane::Graphics
{

#ifdef METHANE_LOGGING_ENABLED
static std::string GetWaitForName(Context::WaitFor wait_for)
{
    META_FUNCTION_TASK();
    switch (wait_for)
    {
    case Context::WaitFor::RenderComplete:      return "WAIT for Render Complete";
    case Context::WaitFor::FramePresented:      return "WAIT for Frame Present";
    case Context::WaitFor::ResourcesUploaded:   return "WAIT for Resources Upload";
    }
    return "";
}
#endif

ContextBase::ContextBase(DeviceBase& device, tf::Executor& parallel_executor, Type type)
    : m_type(type)
    , m_device_ptr(device.GetDevicePtr())
    , m_parallel_executor(parallel_executor)
    , m_resource_manager(*this)
{
    META_FUNCTION_TASK();
}

void ContextBase::RequestDeferredAction(DeferredAction action) const noexcept
{
    META_FUNCTION_TASK();
    m_requested_action = std::max(m_requested_action, action);
}

void ContextBase::CompleteInitialization()
{
    META_FUNCTION_TASK();
    if (m_is_completing_initialization)
        return;

    m_is_completing_initialization = true;
    META_LOG("Complete initialization of context \"" + GetName() + "\"");

    Emit(&IContextCallback::OnContextCompletingInitialization, *this);

    if (m_resource_manager.IsDeferredHeapAllocation())
    {
        WaitForGpu(WaitFor::RenderComplete);
        m_resource_manager.CompleteInitialization();
    }

    UploadResources();

    // Enable deferred heap allocation in case if more resources will be created in runtime
    m_resource_manager.SetDeferredHeapAllocation(true);

    m_requested_action = DeferredAction::None;
    m_is_completing_initialization = false;
}

void ContextBase::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    META_LOG(GetWaitForName(wait_for) + " in context \"" + GetName() + "\"");

    if (wait_for == WaitFor::ResourcesUploaded)
    {
        META_SCOPE_TIMER("ContextBase::WaitForGpu::ResourcesUploaded");
        assert(!!m_upload_fence_ptr);
        OnGpuWaitStart(wait_for);
        m_upload_fence_ptr->FlushOnCpu();
        OnGpuWaitComplete(wait_for);
    }
}

void ContextBase::Reset(Device& device)
{
    META_FUNCTION_TASK();
    META_LOG("RESET context \"" + GetName() + "\" with device adapter \"" + device.GetAdapterName() + "\".");

    WaitForGpu(WaitFor::RenderComplete);
    Release();
    Initialize(static_cast<DeviceBase&>(device), false);
}

void ContextBase::Reset()
{
    META_FUNCTION_TASK();
    META_LOG("RESET context \"" + GetName() + "\"..");

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<DeviceBase> device_ptr = m_device_ptr;
    Release();
    Initialize(*device_ptr, true);
}

void ContextBase::OnGpuWaitComplete(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    if (wait_for != WaitFor::ResourcesUploaded)
    {
        PerformRequestedAction();
    }
}

void ContextBase::Release()
{
    META_FUNCTION_TASK();
    META_LOG("RELEASE context \"" + GetName() + "\"");

    m_device_ptr.reset();
    m_upload_cmd_queue_ptr.reset();
    m_upload_cmd_list_ptr.reset();
    m_upload_cmd_lists_ptr.reset();
    m_upload_fence_ptr.reset();

    Emit(&IContextCallback::OnContextReleased, std::ref(*this));

    m_resource_manager_init_settings.default_heap_sizes         = m_resource_manager.GetDescriptorHeapSizes(true, false);
    m_resource_manager_init_settings.shader_visible_heap_sizes  = m_resource_manager.GetDescriptorHeapSizes(true, true);

    m_resource_manager.Release();
}

void ContextBase::Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    META_LOG("INITIALIZE context \"" + GetName() + "\"");

    m_device_ptr = device.GetDevicePtr();
    m_upload_fence_ptr = Fence::Create(GetUploadCommandQueue());

    const std::string& context_name = GetName();
    if (!context_name.empty())
    {
        m_device_ptr->SetName(context_name + " Device");
    }

    m_resource_manager_init_settings.deferred_heap_allocation = deferred_heap_allocation;
    if (deferred_heap_allocation)
    {
        m_resource_manager_init_settings.default_heap_sizes        = {};
        m_resource_manager_init_settings.shader_visible_heap_sizes = {};
    }

    m_resource_manager.Initialize(m_resource_manager_init_settings);

    if (is_callback_emitted)
    {
        Emit(&IContextCallback::OnContextInitialized, *this);
    }
}

CommandQueue& ContextBase::GetUploadCommandQueue()
{
    META_FUNCTION_TASK();
    if (m_upload_cmd_queue_ptr)
        return *m_upload_cmd_queue_ptr;

    static const std::string s_command_queue_name = "Upload Command Queue";

    m_upload_cmd_queue_ptr = CommandQueue::Create(*this, CommandList::Type::Blit);
    m_upload_cmd_queue_ptr->SetName(s_command_queue_name);

    return *m_upload_cmd_queue_ptr;
}

BlitCommandList& ContextBase::GetUploadCommandList()
{
    META_FUNCTION_TASK();
    if (!m_upload_cmd_list_ptr)
    {
        static const std::string s_command_list_name = "Upload Command List";
        m_upload_cmd_list_ptr = BlitCommandList::Create(GetUploadCommandQueue());
        m_upload_cmd_list_ptr->SetName(s_command_list_name);
    }
    else
    {
        // FIXME: while with wait timeout are used as a workaround for occasional deadlock on command list wait for completion
        //  reproduced at high rate of resource updates (on typography tutorial)
        while(m_upload_cmd_list_ptr->GetState() == CommandList::State::Executing)
            m_upload_cmd_list_ptr->WaitUntilCompleted(16);
    }

    if (m_upload_cmd_list_ptr->GetState() == CommandList::State::Pending)
    {
        META_DEBUG_GROUP_CREATE_VAR(s_debug_region_name, "Upload Resources");
        m_upload_cmd_list_ptr->Reset(s_debug_region_name.get());
    }

    return *m_upload_cmd_list_ptr;
}

CommandListSet& ContextBase::GetUploadCommandListSet()
{
    META_FUNCTION_TASK();
    if (m_upload_cmd_lists_ptr && m_upload_cmd_lists_ptr->GetCount() == 1 &&
        std::addressof((*m_upload_cmd_lists_ptr)[0]) == std::addressof(GetUploadCommandList()))
        return *m_upload_cmd_lists_ptr;

    m_upload_cmd_lists_ptr = CommandListSet::Create({ GetUploadCommandList() });
    return *m_upload_cmd_lists_ptr;
}

Device& ContextBase::GetDevice()
{
    META_FUNCTION_TASK();
    assert(!!m_device_ptr);
    return *m_device_ptr;
}
    
CommandQueueBase& ContextBase::GetUploadCommandQueueBase()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueBase&>(GetUploadCommandQueue());
}

DeviceBase& ContextBase::GetDeviceBase() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<DeviceBase&>(GetDevice());
}

const DeviceBase& ContextBase::GetDeviceBase() const noexcept
{
    META_FUNCTION_TASK();
    assert(!!m_device_ptr);
    return static_cast<const DeviceBase&>(*m_device_ptr);
}

void ContextBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ObjectBase::SetName(name);
    GetDevice().SetName(name + " Device");

    if (m_upload_fence_ptr)
        m_upload_fence_ptr->SetName(name + " Upload Fence");
}

bool ContextBase::UploadResources()
{
    META_FUNCTION_TASK();
    if (!m_upload_cmd_list_ptr)
        return false;

    CommandList::State upload_cmd_state = m_upload_cmd_list_ptr->GetState();
    if (upload_cmd_state == CommandList::State::Pending)
        return false;

    if (upload_cmd_state == CommandList::State::Executing)
        return true;

    if (upload_cmd_state == CommandList::State::Encoding)
        GetUploadCommandList().Commit();

    META_LOG("UPLOAD resources for context \"" + GetName() + "\"");
    GetUploadCommandQueue().Execute(GetUploadCommandListSet());
    return true;
}

void ContextBase::PerformRequestedAction()
{
    META_FUNCTION_TASK();
    switch(m_requested_action)
    {
    case DeferredAction::None: break;
    case DeferredAction::UploadResources:        UploadResources(); break;
    case DeferredAction::CompleteInitialization: CompleteInitialization(); break;
    }
    m_requested_action = DeferredAction::None;
}

void ContextBase::SetDevice(DeviceBase& device)
{
    META_FUNCTION_TASK();
    m_device_ptr = device.GetDevicePtr();
}

Fence& ContextBase::GetUploadFence() const noexcept
{
    assert(m_upload_fence_ptr);
    return *m_upload_fence_ptr;
}

} // namespace Methane::Graphics
