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

FILE: Methane/Graphics/CommandListBase.cpp
Base implementation of the command list interface.

******************************************************************************/

#include "DeviceBase.h"
#include "CommandQueueBase.h"
#include "ProgramBindingsBase.h"
#include "ResourceBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

// Disable debug groups instrumentation with discontinuous CPU frames in Tracy,
// because it is not working for parallel render command lists by some reason
//#define METHANE_DEBUG_GROUP_FRAMES_ENABLED
#ifndef METHANE_DEBUG_GROUP_FRAMES_ENABLED
#undef META_CPU_FRAME_START
#define META_CPU_FRAME_START(name)
#undef META_CPU_FRAME_END
#define META_CPU_FRAME_END(name)
#endif

namespace Methane::Graphics
{

std::string CommandListBase::GetTypeName(Type type) noexcept
{
    META_FUNCTION_TASK();
    switch (type)
    {
    case CommandList::Type::Blit:           return "Blit";
    case CommandList::Type::Render:         return "Render";
    case CommandList::Type::ParallelRender: return "ParallelRender";
    }
    return "Undefined";
}

std::string CommandListBase::GetStateName(State state) noexcept
{
    META_FUNCTION_TASK();
    switch (state)
    {
    case State::Pending:   return "Pending";
    case State::Encoding:  return "Encoding";
    case State::Committed: return "Committed";
    case State::Executing: return "Executing";
    }
    return "Undefined";
}

CommandListBase::DebugGroupBase::DebugGroupBase(std::string name)
    : m_name(std::move(name))
{
    META_FUNCTION_TASK();
}

CommandList::DebugGroup& CommandListBase::DebugGroupBase::AddSubGroup(Data::Index id, std::string name)
{
    META_FUNCTION_TASK();
    if (id >= m_sub_groups.size())
    {
        m_sub_groups.resize(id + 1);
    }

    Ptr<DebugGroup> sp_sub_group = DebugGroup::Create(std::move(name));
    m_sub_groups[id] = sp_sub_group;
    return *sp_sub_group;
}

CommandList::DebugGroup* CommandListBase::DebugGroupBase::GetSubGroup(Data::Index id) const noexcept
{
    META_FUNCTION_TASK();
    return id < m_sub_groups.size() ? m_sub_groups[id].get() : nullptr;
}

CommandListBase::CommandListBase(CommandQueueBase& command_queue, Type type)
    : m_type(type)
    , m_sp_command_queue(command_queue.GetCommandQueuePtr())
    , m_tracy_gpu_scope(TRACY_GPU_SCOPE_INIT(command_queue.GetTracyContext()))
    , m_sp_tracy_construct_location(CREATE_TRACY_SOURCE_LOCATION(GetName().c_str()))
{
    META_FUNCTION_TASK();
    TRACY_GPU_SCOPE_BEGIN_AT_LOCATION(m_tracy_gpu_scope, m_sp_tracy_construct_location.get());
    META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" was created.");
}

CommandListBase::~CommandListBase()
{
    META_FUNCTION_TASK();
    META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" was destroyed.");
}

void CommandListBase::PushDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    META_CPU_FRAME_START(debug_group.GetName().c_str());
    META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" PUSH debug group \"" + debug_group.GetName() + "\"");

    PushOpenDebugGroup(debug_group);
}

void CommandListBase::PopDebugGroup()
{
    META_FUNCTION_TASK();
    if (m_open_debug_groups.empty())
    {
        throw std::underflow_error("Can not pop debug group, since no debug groups were pushed.");
    }

    META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" POP debug group \"" + GetTopOpenDebugGroup()->GetName() + "\"");
    META_CPU_FRAME_END(GetTopOpenDebugGroup()->GetName().c_str());
    m_open_debug_groups.pop();
}

void CommandListBase::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);
    if (m_state == State::Committed || m_state == State::Executing)
        throw std::logic_error("Can not reset command list in committed or executing state.");

    META_LOG(GetTypeName(m_type) + " Command list \"" + GetName() + "\" RESET for commands encoding.");

    SetCommandListStateNoLock(State::Encoding);

    const bool debug_group_changed = GetTopOpenDebugGroup() != p_debug_group;
    if (!m_open_debug_groups.empty() && debug_group_changed)
    {
        PopDebugGroup();
    }

    if (!m_sp_tracy_reset_location)
        m_sp_tracy_reset_location.reset(CREATE_TRACY_SOURCE_LOCATION(GetName().c_str()));
    TRACY_GPU_SCOPE_TRY_BEGIN_AT_LOCATION(m_tracy_gpu_scope, m_sp_tracy_reset_location.get());

    if (p_debug_group && debug_group_changed)
    {
        PushDebugGroup(*p_debug_group);
    }
}

void CommandListBase::SetProgramBindings(ProgramBindings& program_bindings, ProgramBindings::ApplyBehavior::Mask apply_behavior)
{
    META_FUNCTION_TASK();
    VerifyEncodingState();

    ProgramBindingsBase& program_bindings_base = static_cast<ProgramBindingsBase&>(program_bindings);
    program_bindings_base.Apply(*this, apply_behavior);

    if (m_command_state.sp_program_bindings.get() == &program_bindings_base)
        return;

    m_command_state.sp_program_bindings = std::static_pointer_cast<ProgramBindingsBase>(program_bindings_base.GetBasePtr());
    RetainResource(m_command_state.sp_program_bindings);
}

void CommandListBase::Commit()
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);

    if (m_state != State::Encoding)
        throw std::logic_error(GetTypeName() + " Command list \"" + GetName() + "\" in \"" + GetStateName(m_state) + "\" state can not be committed. Only command lists in \"Encoding\" state can be committed.");

    TRACY_GPU_SCOPE_END(m_tracy_gpu_scope);
    META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" COMMIT on frame " + std::to_string(GetCurrentFrameIndex()));

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

    META_LOG("WAIT for completion of " + GetTypeName(m_type) + " Command list \"" + GetName() + "\".");

    if (timeout_ms == 0u)
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

    if (m_state != State::Committed)
        throw std::logic_error(GetTypeName() + " Command list \"" + GetName() + "\" in " + GetStateName(m_state) + " state can not be executed. Only Committed command lists can be executed.");

    if (m_committed_frame_index != frame_index)
        throw std::logic_error(GetTypeName() + " Command list \"" + GetName() + "\" committed on frame " + std::to_string(m_committed_frame_index) + " can not be executed on frame " + std::to_string(frame_index));

    META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" EXECUTE on frame " + std::to_string(frame_index));

    m_completed_callback = completed_callback;

    SetCommandListStateNoLock(State::Executing);
}

void CommandListBase::Complete(uint32_t frame_index)
{
    META_FUNCTION_TASK();
    {
        std::lock_guard<LockableBase(std::mutex)> lock_guard(m_state_mutex);

        if (m_state != State::Executing)
            throw std::logic_error(GetTypeName() + " Command list \"" + GetName() + "\" in " + GetStateName(m_state) + " state can not be completed. Only Executing command lists can be completed.");

        if (m_committed_frame_index != frame_index)
            throw std::logic_error(GetTypeName() + " Command list \"" + GetName() + "\" committed on frame " + std::to_string(m_committed_frame_index) + " can not be completed on frame " + std::to_string(frame_index));

        SetCommandListStateNoLock(State::Pending);
        ResetCommandState();

        TRACY_GPU_SCOPE_COMPLETE(m_tracy_gpu_scope, GetGpuTimeRange(false));
        META_LOG(GetTypeName() + " Command list \"" + GetName() + "\" was COMPLETED on frame " + std::to_string(frame_index) +
                 ", GPU time range: " + static_cast<std::string>(GetGpuTimeRange(true)));
    }

    if (m_completed_callback)
        m_completed_callback(*this);
}

CommandListBase::DebugGroupBase* CommandListBase::GetTopOpenDebugGroup() const
{
    META_FUNCTION_TASK();
    return m_open_debug_groups.empty() ? nullptr : m_open_debug_groups.top().get();
}

void CommandListBase::PushOpenDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    m_open_debug_groups.emplace(static_cast<DebugGroupBase&>(debug_group).GetPtr());
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

    META_LOG(GetTypeName(m_type) + " Command list \"" + GetName() + "\" change state from " + GetStateName(m_state) + " to " + GetStateName(state));

    m_state = state;
    m_state_change_condition_var.notify_one();
}

void CommandListBase::VerifyEncodingState() const
{
    META_FUNCTION_TASK();
    if (m_state != State::Encoding)
    {
        throw std::logic_error(GetTypeName() + " Command list encoding is not possible in \"" + GetStateName(m_state) + "\" state.");
    }
}

bool CommandListBase::IsExecutingOnAnyFrame() const
{
    META_FUNCTION_TASK();
    return m_state == State::Executing;
}

bool CommandListBase::IsCommitted(uint32_t frame_index) const
{
    META_FUNCTION_TASK();
    return m_state == State::Committed && m_committed_frame_index == frame_index;
}

bool CommandListBase::IsExecuting(uint32_t frame_index) const
{
    META_FUNCTION_TASK();
    return m_state == State::Executing && m_committed_frame_index == frame_index;
}

CommandQueue& CommandListBase::GetCommandQueue()
{
    META_FUNCTION_TASK();
    assert(!!m_sp_command_queue);
    return static_cast<CommandQueueBase&>(*m_sp_command_queue);
}

uint32_t CommandListBase::GetCurrentFrameIndex() const
{
    META_FUNCTION_TASK();
    return  GetCommandQueueBase().GetCurrentFrameBufferIndex();
}

void CommandListBase::ResetCommandState()
{
    META_FUNCTION_TASK();
    m_command_state.sp_program_bindings.reset();
    m_command_state.retained_resources.clear();
}

CommandQueueBase& CommandListBase::GetCommandQueueBase() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueBase&>(CommandListBase::GetCommandQueue());
}

const CommandQueueBase& CommandListBase::GetCommandQueueBase() const noexcept
{
    META_FUNCTION_TASK();
    assert(!!m_sp_command_queue);
    return static_cast<const CommandQueueBase&>(*m_sp_command_queue);
}

CommandListSetBase::CommandListSetBase(Refs<CommandList> command_list_refs)
    : m_refs(std::move(command_list_refs))
{
    META_FUNCTION_TASK();
    if (m_refs.empty())
    {
        throw std::invalid_argument("Creating of empty command lists sequence is not allowed.");
    }

    m_base_refs.reserve(m_refs.size());
    m_base_ptrs.reserve(m_refs.size());
    CommandQueue& command_queue = m_refs.front().get().GetCommandQueue();
    for(const Ref<CommandList>& command_list_ref : m_refs)
    {
        CommandListBase& command_list_base = dynamic_cast<CommandListBase&>(command_list_ref.get());
        if (std::addressof(command_list_base.GetCommandQueue()) != std::addressof(command_queue))
        {
            throw std::invalid_argument("All command lists in sequence must be created in one command queue.");
        }

        m_base_refs.emplace_back(command_list_base);
        m_base_ptrs.emplace_back(command_list_base.GetCommandListPtr());
    }
}

CommandList& CommandListSetBase::operator[](Data::Index index) const
{
    META_FUNCTION_TASK();
    if (index > m_refs.size())
        throw std::out_of_range("Command list index " + std::to_string(index) +
                                " is out of collection range (size = " + std::to_string(m_refs.size()) + ").");

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

void CommandListSetBase::Complete() noexcept
{
    META_FUNCTION_TASK();
    for (const Ref<CommandListBase>& command_list_ref : m_base_refs)
    {
        CommandListBase& command_list = command_list_ref.get();
        if (command_list.GetState() != CommandListBase::State::Executing)
            continue;

        try
        {
            command_list_ref.get().Complete(m_executing_on_frame_index);
        }
        catch(std::exception& ex)
        {
            META_UNUSED(ex);
            META_LOG(std::string("Failed to complete command list execution, exception occurred: ") + ex.what());
            assert(false);
        }
    }
}

const CommandListBase& CommandListSetBase::GetCommandListBase(Data::Index index) const
{
    META_FUNCTION_TASK();
    if (index > m_base_refs.size())
        throw std::out_of_range("Command list index " + std::to_string(index) +
                                " is out of collection range (size = " + std::to_string(m_refs.size()) + ").");

    return m_base_refs[index].get();
}

} // namespace Methane::Graphics
