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

class CommandQueueTrackingBase : public CommandQueueBase
{
public:
    CommandQueueTrackingBase(const ContextBase& context, CommandList::Type command_lists_type);
    ~CommandQueueTrackingBase() override;

    // CommandQueue interface
    void Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback = {}) override;

    // Object interface
    void SetName(const std::string& name) override;

    void CompleteExecution(const std::optional<Data::Index>& frame_index = { });

private:
    void WaitForExecution() noexcept;

    const Ptr<CommandListSetBase>& GetNextExecutingCommandListSet() const;
    void CompleteCommandListSetExecution(CommandListSetBase& executing_command_list_set);

    std::queue<Ptr<CommandListSetBase>> m_executing_command_lists;
    mutable TracyLockable(std::mutex,   m_executing_command_lists_mutex)
    TracyLockable(std::mutex,           m_execution_waiting_mutex)
    std::condition_variable_any         m_execution_waiting_condition_var;
    std::atomic<bool>                   m_execution_waiting{ true };
    std::thread                         m_execution_waiting_thread;
    std::exception_ptr                  m_execution_waiting_exception_ptr;
    std::atomic<bool>                   m_name_changed{ false };
};

} // namespace Methane::Graphics
