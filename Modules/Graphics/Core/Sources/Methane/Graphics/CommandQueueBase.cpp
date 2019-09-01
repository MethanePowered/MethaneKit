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

FILE: Methane/Graphics/CommandQueueBase.cpp
Base implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueBase.h"
#include "ContextBase.h"
#include "Instrumentation.h"

#ifdef COMMAND_EXECUTION_LOGGING
#include <Methane/Platform/Utils.h>
#endif

#include <cassert>

namespace Methane
{
namespace Graphics
{

CommandQueueBase::CommandQueueBase(ContextBase& context, bool execution_state_tracking)
    : m_context(context)
    , m_execution_state_tracking(execution_state_tracking)
{
    ITT_FUNCTION_TASK();
}

CommandQueueBase::~CommandQueueBase()
{
    ITT_FUNCTION_TASK();
    std::lock_guard<std::mutex> guard(m_executing_mutex);
    assert(m_executing_on_frames.empty());
}

void CommandQueueBase::Execute(const CommandList::Refs& command_lists)
{
    ITT_FUNCTION_TASK();

    if (m_execution_state_tracking)
    {
        m_executing_mutex.lock();
    }

    const uint32_t frame_index = m_context.GetFrameBufferIndex();
    if (m_execution_state_tracking)
    {
        if (m_executing_on_frames.find(frame_index) != m_executing_on_frames.end())
        {
            m_executing_mutex.unlock();
            return;
        }

#ifdef COMMAND_EXECUTION_LOGGING
        Platform::PrintToDebugOutput("CommandQueue \"" + GetName() + "\" is executing on frame " + std::to_string(frame_index));
#endif

        m_executing_on_frames.insert(frame_index);
    }

    for (const auto& command_list_ref : command_lists)
    {
        if (std::addressof(command_list_ref.get().GetCommandQueue()) != std::addressof(*this))
        {
            throw new std::runtime_error("Can not execute command list created in different command queue.");
        }

        CommandListBase& command_list = dynamic_cast<CommandListBase&>(command_list_ref.get());
        command_list.Execute(frame_index);

        if (m_execution_state_tracking)
        {
            m_executing_command_lists.push_back(command_list.GetPtr());
        }
    }

    if (m_execution_state_tracking)
    {
        m_executing_mutex.unlock();
    }
}

bool CommandQueueBase::IsExecuting(uint32_t frame_index) const
{
    ITT_FUNCTION_TASK();
    if (!m_execution_state_tracking)
        return false;

    std::lock_guard<std::mutex> guard(m_executing_mutex);
    return m_executing_on_frames.find(frame_index) != m_executing_on_frames.end();
}

bool CommandQueueBase::IsExecuting() const
{
    ITT_FUNCTION_TASK();
    return IsExecuting(m_context.GetFrameBufferIndex());
}

void CommandQueueBase::OnCommandListCompleted(CommandListBase& /*command_list*/, uint32_t frame_index)
{
    ITT_FUNCTION_TASK();
    if (!m_execution_state_tracking)
        return;

    std::lock_guard<std::mutex> guard(m_executing_mutex);

    bool all_command_lists_completed = true;
    for (auto executing_command_list_it = m_executing_command_lists.begin();
              executing_command_list_it != m_executing_command_lists.end();)
    {
        CommandListBase::WeakPtr& wp_cmd_list = *executing_command_list_it;
        CommandListBase::Ptr sp_cmd_list = wp_cmd_list.lock();
        if (!sp_cmd_list)
        {
            executing_command_list_it = m_executing_command_lists.erase(executing_command_list_it);
            continue;
        }

        CommandListBase& cmd_list = static_cast<CommandListBase&>(*sp_cmd_list);
        if (cmd_list.IsExecuting(frame_index))
        {
            all_command_lists_completed = false;
            break;
        }
        else if (!cmd_list.IsExecutingOnAnyFrame())
        {
            executing_command_list_it = m_executing_command_lists.erase(executing_command_list_it);
        }
        else
        {
            executing_command_list_it++;
        }
    }
    
    if (all_command_lists_completed)
    {
#ifdef COMMAND_EXECUTION_LOGGING
        Platform::PrintToDebugOutput("CommandQueue \"" + GetName() + "\" execution completed on frame " + std::to_string(frame_index) + "\n\n");
#endif
        m_executing_on_frames.erase(frame_index);
        m_context.OnCommandQueueCompleted(*this, frame_index);
    }
}

} // namespace Graphics
} // namespace Methane