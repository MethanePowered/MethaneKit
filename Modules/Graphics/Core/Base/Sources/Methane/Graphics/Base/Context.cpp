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

FILE: Methane/Graphics/Base/Context.cpp
Base implementation of the context interface.

******************************************************************************/

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/Device.h>
#include <Methane/Graphics/Base/CommandQueue.h>
#include <Methane/Graphics/IDescriptorManager.h>

#include <Methane/Graphics/ICommandKit.h>
#include <Methane/Instrumentation.h>

#include <fmt/format.h>

namespace Methane::Graphics::Base
{

static const std::array<std::string, magic_enum::enum_count<CommandListType>()> g_default_command_kit_names = { {
    "Upload",
    "Render",
    "Parallel Render"
} };

#ifdef METHANE_LOGGING_ENABLED
static const std::array<std::string, magic_enum::enum_count<IContext::WaitFor>()> g_wait_for_names = {{
    "Render Complete",
    "Frame Present",
    "Resources Upload"
}};
#endif

Context::Context(Device& device, UniquePtr<IDescriptorManager>&& descriptor_manager_ptr,
                         tf::Executor& parallel_executor, Type type)
    : m_type(type)
    , m_device_ptr(device.GetPtr<Device>())
    , m_descriptor_manager_ptr(std::move(descriptor_manager_ptr))
    , m_parallel_executor(parallel_executor)
{
    META_FUNCTION_TASK();
}

Context::~Context() = default;

void Context::RequestDeferredAction(DeferredAction action) const noexcept
{
    META_FUNCTION_TASK();
    m_requested_action = std::max(m_requested_action, action);
}

void Context::CompleteInitialization()
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

void Context::WaitForGpu(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' is WAITING for {}", GetName(), g_wait_for_names[magic_enum::enum_index(wait_for).value()]);

    if (wait_for == WaitFor::ResourcesUploaded)
    {
        META_SCOPE_TIMER("Context::WaitForGpu::ResourcesUploaded");
        OnGpuWaitStart(wait_for);
        GetUploadCommandKit().GetFence().FlushOnCpu();
        OnGpuWaitComplete(wait_for);
    }
}

void Context::Reset(IDevice& device)
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' RESET with device adapter '{}'", GetName(), device.GetAdapterName());

    WaitForGpu(WaitFor::RenderComplete);
    Release();
    Initialize(static_cast<Device&>(device), true);
}

void Context::Reset()
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' RESET", GetName());

    WaitForGpu(WaitFor::RenderComplete);

    Ptr<Device> device_ptr = m_device_ptr;
    Release();
    Initialize(*device_ptr, true);
}

void Context::OnGpuWaitStart(WaitFor)
{
    // Intentionally unimplemented
}

void Context::OnGpuWaitComplete(WaitFor wait_for)
{
    META_FUNCTION_TASK();
    if (wait_for != WaitFor::ResourcesUploaded)
    {
        PerformRequestedAction();
    }
}

void Context::Release()
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' RELEASE", GetName());

    m_device_ptr.reset();

    m_default_command_kit_ptr_by_queue.clear();
    for (Ptr<ICommandKit>& cmd_kit_ptr : m_default_command_kit_ptrs)
        cmd_kit_ptr.reset();

    Data::Emitter<IContextCallback>::Emit(&IContextCallback::OnContextReleased, std::ref(*this));
}

void Context::Initialize(Device& device, bool is_callback_emitted)
{
    META_FUNCTION_TASK();
    META_LOG("Context '{}' INITIALIZE", GetName());

    m_device_ptr = device.GetPtr<Device>();
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

ICommandKit& Context::GetDefaultCommandKit(CommandListType type) const
{
    META_FUNCTION_TASK();
    Ptr<ICommandKit>& cmd_kit_ptr = m_default_command_kit_ptrs[magic_enum::enum_index(type).value()];
    if (cmd_kit_ptr)
        return *cmd_kit_ptr;

    cmd_kit_ptr = ICommandKit::Create(*this, type);
    cmd_kit_ptr->SetName(fmt::format("{} {}", GetName(), g_default_command_kit_names[magic_enum::enum_index(type).value()]));

    m_default_command_kit_ptr_by_queue[std::addressof(cmd_kit_ptr->GetQueue())] = cmd_kit_ptr;
    return *cmd_kit_ptr;
}

ICommandKit& Context::GetDefaultCommandKit(ICommandQueue& cmd_queue) const
{
    META_FUNCTION_TASK();
    Ptr<ICommandKit>& cmd_kit_ptr = m_default_command_kit_ptr_by_queue[std::addressof(cmd_queue)];
    if (cmd_kit_ptr)
        return *cmd_kit_ptr;

    cmd_kit_ptr = ICommandKit::Create(cmd_queue);
    return *cmd_kit_ptr;
}

const IDevice& Context::GetDevice() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_device_ptr);
    return *m_device_ptr;
}

Device& Context::GetDeviceBase()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_device_ptr);
    return *m_device_ptr;
}

const Device& Context::GetDeviceBase() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_device_ptr);
    return *m_device_ptr;
}

IDescriptorManager& Context::GetDescriptorManager() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_descriptor_manager_ptr);
    return *m_descriptor_manager_ptr;
}

bool Context::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Object::SetName(name))
        return false;

    GetDeviceBase().SetName(fmt::format("{} Device", name));
    for(const Ptr<ICommandKit>& cmd_kit_ptr : m_default_command_kit_ptrs)
    {
        if (cmd_kit_ptr)
            cmd_kit_ptr->SetName(fmt::format("{} {}", name, g_default_command_kit_names[magic_enum::enum_index(cmd_kit_ptr->GetListType()).value()]));
    }
    return true;
}

template<CommandListPurpose cmd_list_purpose>
void Context::ExecuteSyncCommandLists(const ICommandKit& upload_cmd_kit) const
{
    META_FUNCTION_TASK();
    constexpr auto cmd_list_id = static_cast<CommandListId>(cmd_list_purpose);
    const std::vector<CommandListId> cmd_list_ids = { cmd_list_id };

    for (const auto& [cmd_queue_ptr, cmd_kit_ptr] : m_default_command_kit_ptr_by_queue)
    {
        if (cmd_kit_ptr.get() == std::addressof(upload_cmd_kit) || !cmd_kit_ptr->HasList(cmd_list_id))
            continue;

        ICommandList& cmd_list = cmd_kit_ptr->GetList(cmd_list_id);
        const CommandListState cmd_list_state = cmd_list.GetState();
        if (cmd_list_state == CommandListState::Pending ||
            cmd_list_state == CommandListState::Executing)
            continue;

        if (cmd_list_state == CommandListState::Encoding)
            cmd_list.Commit();

        META_LOG("Context '{}' SYNCHRONIZING resources", GetName());
        ICommandQueue& cmd_queue = cmd_kit_ptr->GetQueue();

        if constexpr (cmd_list_purpose == CommandListPurpose::PreUploadSync)
        {
            // Execute pre-upload synchronization on other queue and wait for sync completion on upload queue
            cmd_queue.Execute(cmd_kit_ptr->GetListSet(cmd_list_ids));
            IFence& cmd_kit_fence = cmd_kit_ptr->GetFence(cmd_list_id);
            cmd_kit_fence.Signal();
            cmd_kit_fence.WaitOnGpu(upload_cmd_kit.GetQueue());
        }
        if constexpr (cmd_list_purpose == CommandListPurpose::PostUploadSync)
        {
            // Wait for upload execution on other queue and execute post-upload synchronization commands on that queue
            IFence& upload_fence = upload_cmd_kit.GetFence(cmd_list_id);
            upload_fence.Signal();
            upload_fence.WaitOnGpu(cmd_queue);
            cmd_queue.Execute(cmd_kit_ptr->GetListSet(cmd_list_ids));
        }
    }
}

bool Context::UploadResources()
{
    META_FUNCTION_TASK();
    const ICommandKit& upload_cmd_kit = GetUploadCommandKit();
    if (!upload_cmd_kit.HasList())
        return false;

    ICommandList& upload_cmd_list = upload_cmd_kit.GetList();
    const CommandListState upload_cmd_state = upload_cmd_list.GetState();
    if (upload_cmd_state == CommandListState::Pending)
        return false;

    if (upload_cmd_state == CommandListState::Executing)
        return true;

    META_LOG("Context '{}' UPLOAD resources", GetName());

    if (upload_cmd_state == CommandListState::Encoding)
        upload_cmd_list.Commit();

    // Execute pre-upload synchronization command lists for all queues except the upload command queue
    // and set upload command queue fence to wait for pre-upload synchronization completion in other command queues
    ExecuteSyncCommandLists<CommandListPurpose::PreUploadSync>(upload_cmd_kit);

    // Execute resource upload command lists
    upload_cmd_kit.GetQueue().Execute(upload_cmd_kit.GetListSet());

    // Execute post-upload synchronization command lists for all queues except the upload command queue
    // and set post-upload command queue fences to wait for upload command command queue completion
    ExecuteSyncCommandLists<CommandListPurpose::PostUploadSync>(upload_cmd_kit);

    return true;
}

void Context::PerformRequestedAction()
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

void Context::SetDevice(Device& device)
{
    META_FUNCTION_TASK();
    m_device_ptr = device.GetPtr<Device>();
}

} // namespace Methane::Graphics::Base
