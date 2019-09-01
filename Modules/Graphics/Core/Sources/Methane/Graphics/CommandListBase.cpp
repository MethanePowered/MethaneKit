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

FILE: Methane/Graphics/CommandListBase.cpp
Base implementation of the command list interface.

******************************************************************************/

#include "CommandListBase.h"
#include "ContextBase.h"
#include "ResourceBase.h"
#include "Instrumentation.h"

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <cassert>

namespace Methane
{
namespace Graphics
{

std::string CommandListBase::GetStateName(State state)
{
    switch (state)
    {
    case State::Pending:   return "Pending";
    case State::Committed: return "Committed";
    case State::Executing: return "Executing";
    }
    return "Undefined";
}

CommandListBase::CommandListBase(CommandQueueBase& command_queue)
    : m_sp_command_queue(command_queue.GetPtr())
{
    ITT_FUNCTION_TASK();
}

void CommandListBase::Commit(bool /*present_drawable*/)
{
    ITT_FUNCTION_TASK();
    std::lock_guard<std::mutex> guard(m_state_mutex);

    if (m_state != State::Pending)
    {
        throw std::logic_error("Command list \"" + GetName() + "\" in " + GetStateName(m_state) + " state can not be committed. Only Pending command lists can be committed.");
    }

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("CommandList \"" + GetName() + "\" is committed on frame " + std::to_string(GetCurrentFrameIndex()));
#endif

    m_committed_frame_index = GetCurrentFrameIndex();
    m_state = State::Committed;

    if (m_pop_debug_group_on_commit)
    {
        PopDebugGroup();
        m_pop_debug_group_on_commit = false;
    }

    // Keep command list from destruction until it's execution is completed
    m_sp_self = shared_from_this();
}

void CommandListBase::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();
    std::lock_guard<std::mutex> guard(m_state_mutex);

    if (m_state != State::Committed)
    {
        throw std::logic_error("Command list \"" + GetName() + "\" in " + GetStateName(m_state) + " state can not be executed. Only Committed command lists can be executed.");
    }

    if (m_committed_frame_index != frame_index)
    {
        throw std::logic_error("Command list \"" + GetName() + "\" committed on frame " + std::to_string(m_committed_frame_index) + " can not be executed on frame " + std::to_string(frame_index));
    }

#ifdef COMMAND_EXECUTION_LOGGING
    Platform::PrintToDebugOutput("CommandList \"" + GetName() + "\" is executing on frame " + std::to_string(frame_index));
#endif

    m_state = State::Executing;
}

void CommandListBase::Complete(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    {
        std::lock_guard<std::mutex> guard(m_state_mutex);

        if (m_state != State::Executing)
        {
            throw std::logic_error("Command list \"" + GetName() + "\" in " + GetStateName(m_state) + " state can not be completed. Only Executing command lists can be completed.");
        }

        if (m_committed_frame_index != frame_index)
        {
            throw std::logic_error("Command list \"" + GetName() + "\" committed on frame " + std::to_string(m_committed_frame_index) + " can not be completed on frame " + std::to_string(frame_index));
        }

#ifdef COMMAND_EXECUTION_LOGGING
        Platform::PrintToDebugOutput("CommandList \"" + GetName() + "\" was completed on frame " + std::to_string(frame_index));
#endif

        m_state = State::Pending;
    }

    GetCommandQueueBase().OnCommandListCompleted(*this, frame_index);

    // Release command list shared pointer, so it can be deleted externally
    m_sp_self.reset();
}

bool CommandListBase::IsExecutingOnAnyFrame() const
{
    ITT_FUNCTION_TASK();
    std::lock_guard<std::mutex> guard(m_state_mutex);
    return m_state == State::Executing;
}

bool CommandListBase::IsCommitted(uint32_t frame_index) const
{
    ITT_FUNCTION_TASK();
    std::lock_guard<std::mutex> guard(m_state_mutex);
    return m_state == State::Committed && m_committed_frame_index == frame_index;
}

bool CommandListBase::IsExecuting(uint32_t frame_index) const
{
    ITT_FUNCTION_TASK();
    std::lock_guard<std::mutex> guard(m_state_mutex);
    return m_state == State::Executing && m_committed_frame_index == frame_index;
}

CommandQueue& CommandListBase::GetCommandQueue()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_command_queue);
    return static_cast<CommandQueueBase&>(*m_sp_command_queue);
}

uint32_t CommandListBase::GetCurrentFrameIndex() const
{
    ITT_FUNCTION_TASK();
    return GetCommandQueueBase().GetContext().GetFrameBufferIndex();
}

void CommandListBase::SetResourceBindings(const Program::ResourceBindings& resource_bindings)
{
    ITT_FUNCTION_TASK();
    resource_bindings.Apply(*this);
}

void CommandListBase::SetResourceTransitionBarriers(const Resource::Refs& resources, ResourceBase::State state_before, ResourceBase::State state_after)
{
    ITT_FUNCTION_TASK();
    ResourceBase::Barriers resource_barriers;
    resource_barriers.reserve(resources.size());
    for (const Resource::Ref& resource_ref : resources)
    {
        resource_barriers.push_back({
            ResourceBase::Barrier::Type::Transition,
            resource_ref.get(),
            state_before,
            state_after
        });
    }
    SetResourceBarriers(resource_barriers);
}

CommandQueueBase& CommandListBase::GetCommandQueueBase()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueBase&>(CommandListBase::GetCommandQueue());
}

const CommandQueueBase& CommandListBase::GetCommandQueueBase() const
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_command_queue);
    return static_cast<const CommandQueueBase&>(*m_sp_command_queue);
}

} // namespace Graphics
} // namespace Methane