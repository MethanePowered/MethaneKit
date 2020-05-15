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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.h
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "QueryBufferDX.h"

#include <Methane/Graphics/CommandQueueBase.h>
#include <Methane/Instrumentation.h>

#include <wrl.h>
#include <d3d12.h>

#include <optional>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IContextDX;
class RenderStateBase;
class CommandListsDX;

class CommandQueueDX final : public CommandQueueBase
{
public:
    CommandQueueDX(ContextBase& context);
    ~CommandQueueDX() override;

    // CommandQueue interface
    void Execute(CommandLists& command_lists, const CommandList::CompletedCallback& completed_callback = {}) override;

    // Object interface
    void SetName(const std::string& name) override;

    void CompleteExecution(const std::optional<Data::Index>& frame_index = { });

    IContextDX&             GetContextDX() noexcept;
    ID3D12CommandQueue&     GetNativeCommandQueue() noexcept;
    TimestampQueryBuffer*   GetTimestampQueryBuffer() noexcept { return m_sp_timestamp_query_buffer.get(); }

private:
    void WaitForExecution() noexcept;

    wrl::ComPtr<ID3D12CommandQueue>   m_cp_command_queue;
    std::queue<Ptr<CommandListsDX>>   m_executing_command_lists;
    mutable TracyLockable(std::mutex, m_executing_command_lists_mutex);
    TracyLockable(std::mutex,         m_execution_waiting_mutex);
    std::condition_variable_any       m_execution_waiting_condition_var;
    std::atomic<bool>                 m_execution_waiting{ true };
    std::thread                       m_execution_waiting_thread;
    Ptr<TimestampQueryBuffer>         m_sp_timestamp_query_buffer;
};

} // namespace Methane::Graphics
