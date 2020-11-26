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

FILE: Methane/Graphics/CommandListBase.cpp
Base implementation of the command list interface.

******************************************************************************/

#include "DeviceBase.h"
#include "CommandQueueBase.h"
#include "ProgramBindingsBase.h"
#include "ResourceBase.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

// Disable debug groups instrumentation with discontinuous CPU frames in Tracy,
// because it is not working for parallel render command lists by some reason
//#define METHANE_DEBUG_GROUP_FRAMES_ENABLED

namespace Methane::Graphics
{

CommandListBase::DebugGroupBase::DebugGroupBase(const std::string& name)
    : ObjectBase(name)
{
    META_FUNCTION_TASK();
}

void CommandListBase::DebugGroupBase::SetName(const std::string&)
{
    META_FUNCTION_NOT_IMPLEMENTED_DESCR("Debug Group can not be renamed");
}

CommandList::DebugGroup& CommandListBase::DebugGroupBase::AddSubGroup(Data::Index id, const std::string& name)
{
    META_FUNCTION_TASK();
    if (id >= m_sub_groups.size())
    {
        m_sub_groups.resize(id + 1);
    }

    Ptr<DebugGroup> sub_group_ptr = DebugGroup::Create(name);
    m_sub_groups[id] = sub_group_ptr;
    return *sub_group_ptr;
}

CommandList::DebugGroup* CommandListBase::DebugGroupBase::GetSubGroup(Data::Index id) const noexcept
{
    META_FUNCTION_TASK();
    return id < m_sub_groups.size() ? m_sub_groups[id].get() : nullptr;
}

CommandListBase::CommandListBase(CommandQueueBase& command_queue, Type type)
    : m_type(type)
    , m_command_queue_ptr(command_queue.GetCommandQueuePtr())
    , m_tracy_gpu_scope(TRACY_GPU_SCOPE_INIT(command_queue.GetTracyContext()))
    , m_tracy_construct_location_ptr(CREATE_TRACY_SOURCE_LOCATION(GetName().c_str()))
{
    META_FUNCTION_TASK();
    TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(m_tracy_gpu_scope, m_tracy_construct_location_ptr.get());
    META_LOG("{} Command list '{}' was created", magic_enum::enum_name(m_type), GetName());
    META_UNUSED(m_tracy_gpu_scope); // silence unused member warning on MacOS when Tracy GPU profiling
}

CommandListBase::~CommandListBase()
{
    META_FUNCTION_TASK();
    META_LOG("{} Command list '{}' was destroyed", magic_enum::enum_name(m_type), GetName());
}

void CommandListBase::PushDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

#ifdef METHANE_DEBUG_GROUP_FRAMES_ENABLED
    META_CPU_FRAME_START(debug_group.GetName().c_str());
#endif
    META_LOG("{} Command list '{}' PUSH debug group '{}'", magic_enum::enum_name(m_type), GetName(), debug_group.GetName());

    PushOpenDebugGroup(debug_group);
}

void CommandListBase::PopDebugGroup()
{
    META_FUNCTION_TASK();
    if (m_open_debug_groups.empty())
    {
        throw std::underflow_error("Can not pop debug group, since no debug groups were pushed");
    }

    META_LOG("{} Command list '{}' POP debug group '{}'", magic_enum::enum_name(m_type), GetName(), GetTopOpenDebugGroup()->GetName());
#ifdef METHANE_DEBUG_GROUP_FRAMES_ENABLED
    META_CPU_FRAME_END(GetTopOpenDebugGroup()->GetName().c_str());
#endif

    m_open_debug_groups.pop();
}

void CommandListBase::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);

    META_CHECK_ARG_DESCR(m_state, m_state != State::Committed && m_state != State::Executing, "can not reset command list in committed or executing state");
    META_LOG("{} Command list '{}' RESET commands encoding", magic_enum::enum_name(m_type), GetName());

    SetCommandListStateNoLock(State::Encoding);

    const bool debug_group_changed = GetTopOpenDebugGroup() != p_debug_group;
    if (!m_open_debug_groups.empty() && debug_group_changed)
    {
        PopDebugGroup();
    }

    if (!m_tracy_reset_location_ptr)
        m_tracy_reset_location_ptr.reset(CREATE_TRACY_SOURCE_LOCATION(GetName().c_str()));
    TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(m_tracy_gpu_scope, m_tracy_reset_location_ptr.get());

    if (p_debug_group && debug_group_changed)
    {
        PushDebugGroup(*p_debug_group);
    }
}

void CommandListBase::SetProgramBindings(ProgramBindings& program_bindings, ProgramBindings::ApplyBehavior apply_behavior)
{
    META_FUNCTION_TASK();
    if (m_command_state.program_bindings_ptr.get() == &program_bindings)
        return;

    auto& program_bindings_base = static_cast<ProgramBindingsBase&>(program_bindings);
    program_bindings_base.Apply(*this, apply_behavior);

    Ptr<ObjectBase> program_bindings_object_ptr = program_bindings_base.GetBasePtr();
    m_command_state.program_bindings_ptr = std::static_pointer_cast<ProgramBindingsBase>(program_bindings_object_ptr);
    RetainResource(program_bindings_object_ptr);
}

void CommandListBase::Commit()
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);

    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Encoding,
                               "{} command list '{}' in {} state can not be committed; only command lists in 'Encoding' state can be committed",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));

    TRACY_GPU_SCOPE_END(m_tracy_gpu_scope);
    META_LOG("{} Command list '{}' COMMIT on frame {}", magic_enum::enum_name(m_type), GetName(), GetCurrentFrameIndex());

    m_committed_frame_index = GetCurrentFrameIndex();

    SetCommandListStateNoLock(State::Committed);

    if (!m_open_debug_groups.empty())
    {
        PopDebugGroup();
    }
}

void CommandListBase::WaitUntilCompleted(uint32_t timeout_ms)
{
    META_FUNCTION_TASK();
    std::unique_lock<LockableBase(std::mutex)> pending_state_lock(m_state_change_mutex);
    const auto is_completed = [this] { return m_state != State::Executing; };
    if (is_completed())
        return;

    META_LOG("{} Command list '{}' WAITING for completion", magic_enum::enum_name(m_type), GetName());

    if (timeout_ms == 0U)
    {
        m_state_change_condition_var.wait(pending_state_lock, is_completed);
    }
    else
    {
        m_state_change_condition_var.wait_for(pending_state_lock, std::chrono::milliseconds(timeout_ms), is_completed);
    }
}

void CommandListBase::Execute(uint32_t frame_index, const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);

    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Committed,
                               "{} command list '{}' in {} state can not be executed; only command lists in 'Committed' state can be executed",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));

    META_CHECK_ARG_EQUAL_DESCR(frame_index, m_committed_frame_index,
                               "{} command list '{}' committed on frame {} can not be executed on frame {}",
                               magic_enum::enum_name(m_type), GetName(), m_committed_frame_index, frame_index);

    META_LOG("{} Command list '{}' EXECUTE on frame {}", magic_enum::enum_name(m_type), GetName(), frame_index);

    m_completed_callback = completed_callback;

    SetCommandListStateNoLock(State::Executing);
}

void CommandListBase::Complete(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    CompleteInternal(frame_index);

    if (m_completed_callback)
        m_completed_callback(*this);
}

void CommandListBase::CompleteInternal(uint32_t frame_index)
{
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);

    META_CHECK_ARG_EQUAL_DESCR(m_state, State::Executing,
                               "{} command list '{}' in {} state can not be completed; only command lists in 'Executing' state can be completed",
                               magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state));

    META_CHECK_ARG_EQUAL_DESCR(frame_index, m_committed_frame_index,
                               "{} command list '{}' committed on frame {} can not be completed on frame {}",
                               magic_enum::enum_name(m_type), GetName(), m_committed_frame_index, frame_index);

    SetCommandListStateNoLock(State::Pending);
    ResetCommandState();

    TRACY_GPU_SCOPE_COMPLETE(m_tracy_gpu_scope, GetGpuTimeRange(false));
    META_LOG("{} Command list '{}' was COMPLETED on frame {} with GPU timings {}", magic_enum::enum_name(m_type), GetName(), frame_index, static_cast<std::string>(GetGpuTimeRange(true)));
}

CommandListBase::DebugGroupBase* CommandListBase::GetTopOpenDebugGroup() const
{
    META_FUNCTION_TASK();
    return m_open_debug_groups.empty() ? nullptr : m_open_debug_groups.top().get();
}

void CommandListBase::PushOpenDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    m_open_debug_groups.emplace(std::static_pointer_cast<DebugGroupBase>(static_cast<DebugGroupBase&>(debug_group).GetBasePtr()));
}

void CommandListBase::ClearOpenDebugGroups()
{
    META_FUNCTION_TASK();
    while(!m_open_debug_groups.empty())
    {
        m_open_debug_groups.pop();
    }
}

void CommandListBase::SetCommandListState(State state)
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);
    SetCommandListStateNoLock(state);
}

void CommandListBase::SetCommandListStateNoLock(State state)
{
    META_FUNCTION_TASK();
    if (m_state == state)
        return;

    META_LOG("{} Command list '{}' change state from {} to {}", magic_enum::enum_name(m_type), GetName(), magic_enum::enum_name(m_state), GetStateName(state));

    m_state = state;
    m_state_change_condition_var.notify_one();
}

CommandQueue& CommandListBase::GetCommandQueue()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_command_queue_ptr);
    return static_cast<CommandQueueBase&>(*m_command_queue_ptr);
}

uint32_t CommandListBase::GetCurrentFrameIndex() const
{
    META_FUNCTION_TASK();
    return  GetCommandQueueBase().GetCurrentFrameBufferIndex();
}

void CommandListBase::ResetCommandState()
{
    META_FUNCTION_TASK();
    m_command_state.program_bindings_ptr.reset();
    m_command_state.retained_resources.clear();
}

CommandQueueBase& CommandListBase::GetCommandQueueBase()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueBase&>(CommandListBase::GetCommandQueue());
}

const CommandQueueBase& CommandListBase::GetCommandQueueBase() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_command_queue_ptr);
    return static_cast<const CommandQueueBase&>(*m_command_queue_ptr);
}

CommandListSetBase::CommandListSetBase(const Refs<CommandList>& command_list_refs)
    : m_refs(command_list_refs)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY_DESCR(command_list_refs, "creating of empty command lists set is not allowed.");

    m_base_refs.reserve(m_refs.size());
    m_base_ptrs.reserve(m_refs.size());

    for(const Ref<CommandList>& command_list_ref : m_refs)
    {
        auto& command_list_base = dynamic_cast<CommandListBase&>(command_list_ref.get());
        META_CHECK_ARG_NAME_DESCR("command_list_refs",
                                  std::addressof(command_list_base.GetCommandQueue()) == std::addressof(m_refs.front().get().GetCommandQueue()),
                                  "all command lists in set must be created in one command queue");

        m_base_refs.emplace_back(command_list_base);
        m_base_ptrs.emplace_back(command_list_base.GetCommandListPtr());
    }
}

CommandList& CommandListSetBase::operator[](Data::Index index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(index, m_refs.size());

    return m_refs[index].get();
}

void CommandListSetBase::Execute(Data::Index frame_index, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    m_executing_on_frame_index = frame_index;
    for (const Ref<CommandListBase>& command_list_ref : m_base_refs)
    {
        command_list_ref.get().Execute(frame_index, completed_callback);
    }
}

void CommandListSetBase::Complete() const
{
    META_FUNCTION_TASK();
    for (const Ref<CommandListBase>& command_list_ref : m_base_refs)
    {
        CommandListBase& command_list = command_list_ref.get();
        if (command_list.GetState() != CommandListBase::State::Executing)
            continue;

        command_list.Complete(m_executing_on_frame_index);
    }
}

const CommandListBase& CommandListSetBase::GetCommandListBase(Data::Index index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS(index, m_base_refs.size());
    return m_base_refs[index].get();
}

} // namespace Methane::Graphics
