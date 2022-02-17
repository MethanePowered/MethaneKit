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

FILE: Methane/Graphics/ContextBase.cpp
Base implementation of the context interface.

******************************************************************************/

#include "ContextBase.h"
#include "DeviceBase.h"
#include "CommandQueueBase.h"
#include "DescriptorManager.h"

#include <Methane/Graphics/CommandKit.h>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>

namespace Methane::Graphics
{

static const std::array<std::string, magic_enum::enum_count<CommandList::Type>()> g_default_command_kit_names = { {
    "Upload",
    "Render",
    "Parallel Render"
} };

#ifdef METHANE_LOGGING_ENABLED
static const std::array<std::string, magic_enum::enum_count<Context::WaitFor>()> g_wait_for_names = {{
    "Render Complete",
    "Frame Present",
    "Resources Upload"
}};
#endif

ContextBase::ContextBase(DeviceBase& device, UniquePtr<DescriptorManager>&& descriptor_manager_ptr,
                         tf::Executor& parallel_executor, Type type)
    : m_type(type)
    , m_device_ptr(device.GetPtr<DeviceBase>())
    , m_descriptor_manager_ptr(std::move(descriptor_manager_ptr))
    , m_parallel_executor(parallel_executor)
{
    META_FUNCTION_TASK();
}

ContextBase::~ContextBase() = default;

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
    META_LOG("Complete initialization of context '{}'", GetName());

    Data::Emitter<IContextCallback>::Emit(&IContextCallback::OnContextCompletingInitialization, *this);
    UploadResources();
    GetDescriptorManager().CompleteInitialization();

    m_requested_action             = DeferredAction::None;
    m_is_completing_initialization = false;
}

void ContextBase::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' is WAITING for {}", GetName(), g_wait_for_names[magic_enum::enum_index(wait_for).value()]);

    if (wait_for == WaitFor::ResourcesUploaded)
    {
        META_SCOPE_TIMER("ContextBase::WaitForGpu::ResourcesUploaded");
        OnGpuWaitStart(wait_for);
        GetUploadCommandKit().GetFence().FlushOnCpu();
        OnGpuWaitComplete(wait_for);
    }
}

void ContextBase::Reset(Device& device)
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' RESET with device adapter '{}'", GetName(), device.GetAdapterName());

    WaitForGpu(WaitFor::RenderComplete);
    Release();
    Initialize(static_cast<DeviceBase&>(device), false);
}

void ContextBase::Reset()
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' RESET", GetName());

    WaitForGpu(WaitFor::RenderComplete);

    Ptr <DeviceBase> device_ptr = m_device_ptr;
    Release();
    Initialize(*device_ptr, true);
}

void ContextBase::OnGpuWaitStart(WaitFor)
{
    // Intentionally unimplemented
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
    META_LOG("Context '{}' RELEASE", GetName());

    m_descriptor_manager_ptr.reset();
    m_device_ptr.reset();

    m_default_command_kit_ptr_by_queue.clear();
    for (Ptr<CommandKit>& cmd_kit_ptr : m_default_command_kit_ptrs)
        cmd_kit_ptr.reset();

    Data::Emitter<IContextCallback>::Emit(&IContextCallback::OnContextReleased, std::ref(*this));
}

void ContextBase::Initialize(DeviceBase& device, bool /* deferred_heap_allocation */, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' INITIALIZE", GetName());

    m_device_ptr = device.GetPtr<DeviceBase>();
    if (const std::string& context_name = GetName();
        !context_name.empty())
    {
        m_device_ptr->SetName(fmt::format("{} Device", context_name));
    }

    if (is_callback_emitted)
    {
        Data::Emitter<IContextCallback>::Emit(&IContextCallback::OnContextInitialized, *this);
    }
}

CommandKit& ContextBase::GetDefaultCommandKit(CommandList::Type type) const
{
    META_FUNCTION_TASK();
    Ptr<CommandKit>& cmd_kit_ptr = m_default_command_kit_ptrs[magic_enum::enum_index(type).value()];
    if (cmd_kit_ptr)
        return *cmd_kit_ptr;

    cmd_kit_ptr = CommandKit::Create(*this, type);
    cmd_kit_ptr->SetName(fmt::format("{} {}", GetName(), g_default_command_kit_names[magic_enum::enum_index(type).value()]));

    m_default_command_kit_ptr_by_queue[std::addressof(cmd_kit_ptr->GetQueue())] = cmd_kit_ptr;
    return *cmd_kit_ptr;
}

CommandKit& ContextBase::GetDefaultCommandKit(CommandQueue& cmd_queue) const
{
    META_FUNCTION_TASK();
    Ptr<CommandKit>& cmd_kit_ptr = m_default_command_kit_ptr_by_queue[std::addressof(cmd_queue)];
    if (cmd_kit_ptr)
        return *cmd_kit_ptr;

    cmd_kit_ptr = CommandKit::Create(cmd_queue);
    return *cmd_kit_ptr;
}

const Device& ContextBase::GetDevice() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_device_ptr);
    return *m_device_ptr;
}

DeviceBase& ContextBase::GetDeviceBase()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_device_ptr);
    return *m_device_ptr;
}

const DeviceBase& ContextBase::GetDeviceBase() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_device_ptr);
    return *m_device_ptr;
}

DescriptorManager& ContextBase::GetDescriptorManager() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_descriptor_manager_ptr);
    return *m_descriptor_manager_ptr;
}

bool ContextBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!ObjectBase::SetName(name))
        return false;

    GetDeviceBase().SetName(fmt::format("{} Device", name));
    for(const Ptr<CommandKit>& cmd_kit_ptr : m_default_command_kit_ptrs)
    {
        if (cmd_kit_ptr)
            cmd_kit_ptr->SetName(fmt::format("{} {}", name, g_default_command_kit_names[magic_enum::enum_index(cmd_kit_ptr->GetListType()).value()]));
    }
    return true;
}

bool ContextBase::UploadResources()
{
    META_FUNCTION_TASK();
    CommandKit& upload_cmd_kit = GetUploadCommandKit();
    if (!upload_cmd_kit.HasList())
        return false;

    // Execute all default command lists for all queues except the upload command list
    bool is_resources_synchronization = false;
    for(const auto& [cmd_queue_ptr, cmd_kit_ptr] : m_default_command_kit_ptr_by_queue)
    {
        if (cmd_kit_ptr.get() == std::addressof(upload_cmd_kit) || !cmd_kit_ptr->HasList())
            continue;

        CommandList& cmd_list = cmd_kit_ptr->GetList();
        const CommandList::State cmd_list_state = cmd_list.GetState();
        if (cmd_list_state == CommandList::State::Pending ||
            cmd_list_state == CommandList::State::Executing)
            continue;

        if (cmd_list_state == CommandList::State::Encoding)
            cmd_list.Commit();

        META_LOG("Context '{}' SYNCHRONIZING resources", GetName());
        cmd_kit_ptr->GetQueue().Execute(cmd_kit_ptr->GetListSet());
        cmd_kit_ptr->GetFence().Signal();
        is_resources_synchronization = true;
    }

    CommandList& upload_cmd_list = upload_cmd_kit.GetList();
    const CommandList::State upload_cmd_state = upload_cmd_list.GetState();
    if (upload_cmd_state == CommandList::State::Pending)
        return false;

    if (upload_cmd_state == CommandList::State::Executing)
        return true;

    if (upload_cmd_state == CommandList::State::Encoding)
        upload_cmd_list.Commit();

    if (is_resources_synchronization)
    {
        // Upload command queue waits for resources synchronization completion in other command queues
        for(const auto& [cmd_queue_ptr, cmd_kit_ptr] : m_default_command_kit_ptr_by_queue)
        {
            if (cmd_kit_ptr.get() == std::addressof(upload_cmd_kit) || !cmd_kit_ptr->HasList())
                continue;

            cmd_kit_ptr->GetFence().WaitOnGpu(upload_cmd_kit.GetQueue());
        }
    }

    META_LOG("Context '{}' UPLOAD resources", GetName());
    upload_cmd_kit.GetQueue().Execute(upload_cmd_kit.GetListSet());
    return true;
}

void ContextBase::PerformRequestedAction()
{
    META_FUNCTION_TASK();
    switch(m_requested_action)
    {
    case DeferredAction::None:                   break;
    case DeferredAction::UploadResources:        UploadResources(); break;
    case DeferredAction::CompleteInitialization: CompleteInitialization(); break;
    default:                                     META_UNEXPECTED_ARG(m_requested_action);
    }
    m_requested_action = DeferredAction::None;
}

void ContextBase::SetDevice(DeviceBase& device)
{
    META_FUNCTION_TASK();
    m_device_ptr = device.GetPtr<DeviceBase>();
}

} // namespace Methane::Graphics
