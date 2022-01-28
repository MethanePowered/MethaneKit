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

FILE: Methane/Graphics/CommandQueueTrackingBase.h
Base implementation of the command queue with execution tracking.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandQueueBase.h>
#include <Methane/Instrumentation.h>

#include <optional>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <exception>

namespace Methane::Graphics
{

class CommandQueueTrackingBase // NOSONAR - destructor is required
    : public CommandQueueBase
{
public:
    CommandQueueTrackingBase(const ContextBase& context, CommandList::Type command_lists_type);
    ~CommandQueueTrackingBase() override;

    // CommandQueue interface
    void Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback = {}) override;

    // Object interface
    bool SetName(const std::string& name) override;

    void CompleteExecution(const Opt<Data::Index>& frame_index = { });

    Ptr<CommandListSetBase> GetLastExecutingCommandListSet() const;

protected:
    using ExecutingCommandListSets = std::queue<Ptr<CommandListSetBase>>;

    template<typename MutexType>
    class ExecutingCommandListSetsGuard
    {
    public:
        ExecutingCommandListSetsGuard(const ExecutingCommandListSets& executing_command_lists, MutexType& mutex)
            : m_lock_guard(mutex)
            , m_executing_command_lists(executing_command_lists)
        { }

        const ExecutingCommandListSets& GetExecutingCommandLists() const noexcept { return m_executing_command_lists; }

    private:
        std::scoped_lock<MutexType>     m_lock_guard;
        const ExecutingCommandListSets& m_executing_command_lists;
    };

    decltype(auto) GetExecutingCommandListsGuard() const
    {
        return ExecutingCommandListSetsGuard<decltype(m_executing_command_lists_mutex)>(m_executing_command_lists, m_executing_command_lists_mutex);
    }

private:
    void WaitForExecution() noexcept;

    const Ptr<CommandListSetBase>& GetNextExecutingCommandListSet() const;
    void CompleteCommandListSetExecution(CommandListSetBase& executing_command_list_set);

    ExecutingCommandListSets            m_executing_command_lists;
    mutable TracyLockable(std::mutex,   m_executing_command_lists_mutex)
    TracyLockable(std::mutex,           m_execution_waiting_mutex)
    std::condition_variable_any         m_execution_waiting_condition_var;
    std::atomic<bool>                   m_execution_waiting{ true };
    std::thread                         m_execution_waiting_thread;
    std::exception_ptr                  m_execution_waiting_exception_ptr;
    std::atomic<bool>                   m_name_changed{ true };
};

} // namespace Methane::Graphics
