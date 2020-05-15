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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.cpp
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueDX.h"
#include "CommandListDX.h"
#include "DeviceDX.h"
#include "BlitCommandListDX.h"
#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"
#include "QueryBufferDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/Primitives.h>

#include <nowide/convert.hpp>
#include <cassert>

namespace Methane::Graphics
{

constexpr uint32_t g_max_timestamp_queries_count_per_frame = 1000;

Ptr<CommandQueue> CommandQueue::Create(Context& context)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandQueueDX>(dynamic_cast<ContextBase&>(context));
}

static wrl::ComPtr<ID3D12CommandQueue> CreateNativeCommandQueue(const DeviceDX& device)
{
    META_FUNCTION_TASK();
    const wrl::ComPtr<ID3D12Device>& cp_device = device.GetNativeDevice();
    assert(!!cp_device);

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    wrl::ComPtr<ID3D12CommandQueue> cp_command_queue;
    ThrowIfFailed(cp_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&cp_command_queue)), cp_device.Get());
    return cp_command_queue;
}

CommandQueueDX::CommandQueueDX(ContextBase& context)
    : CommandQueueBase(context)
    , m_cp_command_queue(CreateNativeCommandQueue(GetContextDX().GetDeviceDX()))
    , m_execution_waiting_thread(&CommandQueueDX::WaitForExecution, this)
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    , m_sp_timestamp_query_buffer(TimestampQueryBuffer::Create(*this, g_max_timestamp_queries_count_per_frame))
#endif
{
    META_FUNCTION_TASK();
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    TimestampQueryBufferDX& timestamp_query_buffer_dx = static_cast<TimestampQueryBufferDX&>(*m_sp_timestamp_query_buffer);
    InitializeTracyGpuContext(
        Tracy::GpuContext::Settings(
            timestamp_query_buffer_dx.GetGpuCalibrationTimestamp(),
            Data::ConvertFrequencyToTickPeriod(timestamp_query_buffer_dx.GetGpuFrequency())
        )
    );
#endif
}

CommandQueueDX::~CommandQueueDX()
{
    META_FUNCTION_TASK();
    m_execution_waiting = false;
    m_execution_waiting_condition_var.notify_one();
    m_execution_waiting_thread.join();
}

void CommandQueueDX::Execute(CommandLists& command_lists, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();

    CommandQueueBase::Execute(command_lists, completed_callback);

    CommandListsDX& dx_command_lists = static_cast<CommandListsDX&>(command_lists);
    {
        std::lock_guard<LockableBase(std::mutex)> lock_guard(m_executing_command_lists_mutex);
        m_executing_command_lists.push(std::static_pointer_cast<CommandListsDX>(dx_command_lists.GetPtr()));
        m_execution_waiting_condition_var.notify_one();
    }
}

void CommandQueueDX::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    CommandQueueBase::SetName(name);
    m_cp_command_queue->SetName(nowide::widen(name).c_str());
}

void CommandQueueDX::CompleteExecution(const std::optional<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_executing_command_lists_mutex);
    while (!m_executing_command_lists.empty() &&
          (!frame_index.has_value() || m_executing_command_lists.front()->GetExecutingOnFrameIndex() == *frame_index))
    {
        m_executing_command_lists.front()->Complete();
        m_executing_command_lists.pop();
    }
    m_execution_waiting_condition_var.notify_one();
}

IContextDX& CommandQueueDX::GetContextDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextDX&>(GetContext());
}

ID3D12CommandQueue& CommandQueueDX::GetNativeCommandQueue() noexcept
{
    META_FUNCTION_TASK();
    assert(!!m_cp_command_queue);
    return *m_cp_command_queue.Get();
}

void CommandQueueDX::WaitForExecution() noexcept
{
    do
    {
        std::unique_lock<LockableBase(std::mutex)> lock(m_execution_waiting_mutex);
        m_execution_waiting_condition_var.wait(lock,
            [this]{ return !m_execution_waiting || !m_executing_command_lists.empty(); }
        );

        while(!m_executing_command_lists.empty())
        {
            CommandListsDX* p_command_lists = nullptr;
            {
                std::lock_guard<LockableBase(std::mutex)> lock_guard(m_executing_command_lists_mutex);
                if (m_executing_command_lists.empty())
                    break;
                p_command_lists = m_executing_command_lists.front().get();
            }
            assert(p_command_lists);
            p_command_lists->GetExecutionCompletedFenceDX().Wait();
            {
                std::lock_guard<LockableBase(std::mutex)> lock_guard(m_executing_command_lists_mutex);
                if (!m_executing_command_lists.empty() && m_executing_command_lists.front().get() == p_command_lists)
                {
                    p_command_lists->Complete();
                    m_executing_command_lists.pop();
                }
            }
        }
    }
    while(m_execution_waiting);
}

} // namespace Methane::Graphics
