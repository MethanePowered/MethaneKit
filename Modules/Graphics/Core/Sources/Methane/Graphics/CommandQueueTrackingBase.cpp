/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/CommandQueueTrackingBase.cpp
Base implementation of the command queue with execution tracking.

******************************************************************************/

#include "CommandQueueTrackingBase.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <stdexcept>
#include <cassert>

namespace Methane::Graphics
{

CommandQueueTrackingBase::CommandQueueTrackingBase(const ContextBase& context, CommandList::Type command_lists_type)
    : CommandQueueBase(context, command_lists_type)
    , m_execution_waiting_thread(&CommandQueueTrackingBase::WaitForExecution, this)
{
    META_FUNCTION_TASK();
}

CommandQueueTrackingBase::~CommandQueueTrackingBase()
{
    META_FUNCTION_TASK();
    try
    {
        CompleteExecution();
    }
    catch(const std::exception& ex)
    {
        META_UNUSED(ex);
        META_LOG("WARNING: Command queue '{}' has failed to complete command list execution, exception occurred: {}", GetName(), ex.what());
        assert(false);
    }
    m_execution_waiting = false;
    m_execution_waiting_condition_var.notify_one();
    m_execution_waiting_thread.join();
}

void CommandQueueTrackingBase::Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandQueueBase::Execute(command_lists, completed_callback);

    if (!m_execution_waiting)
    {
        m_execution_waiting_thread.join();
        META_CHECK_ARG_NOT_NULL_DESCR(m_execution_waiting_exception_ptr, "Command queue '{}' execution waiting thread has unexpectedly finished", GetName());
        std::rethrow_exception(m_execution_waiting_exception_ptr);
    }

    auto& dx_command_lists = static_cast<CommandListSetBase&>(command_lists);
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    m_executing_command_lists.push(dx_command_lists.GetPtr());
    m_execution_waiting_condition_var.notify_one();
}

void CommandQueueTrackingBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (name == GetName())
        return;

    CommandQueueBase::SetName(name);
    m_name_changed = true;
}

void CommandQueueTrackingBase::CompleteExecution(const std::optional<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    while (!m_executing_command_lists.empty() &&
          (!frame_index.has_value() || m_executing_command_lists.front()->GetExecutingOnFrameIndex() == *frame_index))
    {
        m_executing_command_lists.front()->Complete();
        m_executing_command_lists.pop();
    }
    m_execution_waiting_condition_var.notify_one();
}

void CommandQueueTrackingBase::WaitForExecution() noexcept
{
    try
    {
        do
        {
            std::unique_lock lock(m_execution_waiting_mutex);
            m_execution_waiting_condition_var.wait(lock,
                [this] { return !m_execution_waiting || !m_executing_command_lists.empty(); }
            );

            if (m_name_changed)
            {
                const std::string thread_name = fmt::format("{} Wait for Execution", GetName());
                META_THREAD_NAME(thread_name.c_str());
                m_name_changed = false;
            }

            while (!m_executing_command_lists.empty())
            {
                const Ptr<CommandListSetBase>& command_list_set_ptr = GetNextExecutingCommandListSet();
                if (!command_list_set_ptr)
                    break;

                command_list_set_ptr->WaitUntilCompleted();
                CompleteCommandListSetExecution(*command_list_set_ptr);
            }
        }
        while (m_execution_waiting);
    }
    catch (...)
    {
        m_execution_waiting_exception_ptr = std::current_exception();
        m_execution_waiting = false;
    }
}

const Ptr<CommandListSetBase>& CommandQueueTrackingBase::GetNextExecutingCommandListSet() const
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);

    static const Ptr<CommandListSetBase> s_empty_command_list_set_ptr;
    if (m_executing_command_lists.empty())
        return s_empty_command_list_set_ptr;

    META_CHECK_ARG_NOT_NULL(m_executing_command_lists.front());
    return m_executing_command_lists.front();
}

void CommandQueueTrackingBase::CompleteCommandListSetExecution(CommandListSetBase& executing_command_list_set)
{
    META_FUNCTION_TASK();
    std::unique_lock lock_guard(m_executing_command_lists_mutex);

    if (!m_executing_command_lists.empty() && m_executing_command_lists.front().get() == std::addressof(executing_command_list_set))
    {
        m_executing_command_lists.pop();
    }
}

} // namespace Methane::Graphics
