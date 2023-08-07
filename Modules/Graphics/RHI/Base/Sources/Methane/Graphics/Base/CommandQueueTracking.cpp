/******************************************************************************

Copyright 2021-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/CommandQueueTracking.cpp
Base implementation of the command queue with execution tracking.

******************************************************************************/

#include <Methane/Graphics/Base/CommandQueueTracking.h>
#include <Methane/Graphics/Base/CommandListSet.h>
#include <Methane/Graphics/Base/Context.h>

#include <Methane/Graphics/RHI/IQueryPool.h>
#include <Methane/Graphics/RHI/ISystem.h>

#include <Methane/TracyGpu.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <stdexcept>
#include <cassert>

namespace Methane::Graphics::Base
{

static Tracy::GpuContext::Type ConvertSystemGraphicsApiToTracyGpuContextType(Rhi::NativeApi graphics_api)
{
    META_FUNCTION_TASK();
    switch(graphics_api)
    {
    case Rhi::NativeApi::Undefined:    return Tracy::GpuContext::Type::Undefined;
    case Rhi::NativeApi::DirectX:      return Tracy::GpuContext::Type::DirectX12;
    case Rhi::NativeApi::Vulkan:       return Tracy::GpuContext::Type::Vulkan;
    case Rhi::NativeApi::Metal:        return Tracy::GpuContext::Type::Metal;
    default: META_UNEXPECTED_ARG_RETURN(graphics_api, Tracy::GpuContext::Type::Undefined);
    }
};

CommandQueueTracking::CommandQueueTracking(const Context& context, Rhi::CommandListType command_lists_type)
    : CommandQueue(context, command_lists_type)
    , m_execution_waiting_thread(&CommandQueueTracking::WaitForExecution, this)
{ }

CommandQueueTracking::~CommandQueueTracking()
{
    META_FUNCTION_TASK();
    ShutdownQueueExecution();
}

void CommandQueueTracking::InitializeTimestampQueryPool()
{
    META_FUNCTION_TASK();
    constexpr uint32_t g_max_timestamp_queries_count_per_frame = 1000;
    m_timestamp_query_pool_ptr = Rhi::ITimestampQueryPool::Create(*this, g_max_timestamp_queries_count_per_frame);
    if (!m_timestamp_query_pool_ptr)
        return;

    const Rhi::ITimestampQueryPool::CalibratedTimestamps& calibrated_timestamps = m_timestamp_query_pool_ptr->GetCalibratedTimestamps();
    InitializeTracyGpuContext(
        Tracy::GpuContext::Settings(
            ConvertSystemGraphicsApiToTracyGpuContextType(Rhi::ISystem::GetNativeApi()),
            calibrated_timestamps.cpu_ts,
            calibrated_timestamps.gpu_ts,
            Data::ConvertFrequencyToTickPeriod(m_timestamp_query_pool_ptr->GetGpuFrequency())
        )
    );
}

void CommandQueueTracking::Execute(Rhi::ICommandListSet& command_lists, const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandQueue::Execute(command_lists, completed_callback);

    if (!m_execution_waiting)
    {
        m_execution_waiting_thread.join();
        META_CHECK_ARG_NOT_NULL_DESCR(m_execution_waiting_exception_ptr, "Command queue '{}' execution waiting thread has unexpectedly finished", GetName());
        if (m_execution_waiting_exception_ptr)
            std::rethrow_exception(m_execution_waiting_exception_ptr);
    }

    auto& command_lists_base = static_cast<CommandListSet&>(command_lists);
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    m_executing_command_lists.push(command_lists_base.GetBasePtr());
    m_execution_waiting_condition_var.notify_one();
}

bool CommandQueueTracking::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!CommandQueue::SetName(name))
        return false;

    m_name_changed = true;
    return true;
}

void CommandQueueTracking::CompleteExecution(const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    while (!m_executing_command_lists.empty() &&
           m_executing_command_lists.front()->GetFrameIndex() == frame_index)
    {
        m_executing_command_lists.front()->Complete();
        m_executing_command_lists.pop();
    }
    m_execution_waiting_condition_var.notify_one();
}

void CommandQueueTracking::WaitForExecution() noexcept
{
    try
    {
        do
        {
            std::unique_lock lock(m_execution_waiting_mutex);
            m_execution_waiting_condition_var.wait_for(lock, std::chrono::milliseconds(32),
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
                Ptr<CommandListSet> command_list_set_ptr = GetNextExecutingCommandListSet();
                if (!command_list_set_ptr)
                    break;

                command_list_set_ptr->WaitUntilCompleted();
                CompleteCommandListSetExecution(*command_list_set_ptr);
            }

            if (m_timestamp_query_pool_ptr)
            {
                const Rhi::ITimestampQueryPool::CalibratedTimestamps calibrated_timestamps = m_timestamp_query_pool_ptr->Calibrate();
                GetTracyContext().Calibrate(calibrated_timestamps.cpu_ts, calibrated_timestamps.gpu_ts);
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

Ptr<CommandListSet> CommandQueueTracking::GetLastExecutingCommandListSet() const
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    return m_executing_command_lists.empty() ? Ptr<CommandListSet>() : m_executing_command_lists.back();
}

const Ptr<Rhi::ITimestampQueryPool>& CommandQueueTracking::GetTimestampQueryPoolPtr()
{
    META_FUNCTION_TASK();
    if (!m_timestamp_query_pool_ptr)
        InitializeTimestampQueryPool();

    return m_timestamp_query_pool_ptr;
}

const Ptr<CommandListSet>& CommandQueueTracking::GetNextExecutingCommandListSet() const
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);

    static const Ptr<CommandListSet> s_empty_command_list_set_ptr;
    if (m_executing_command_lists.empty())
        return s_empty_command_list_set_ptr;

    META_CHECK_ARG_NOT_NULL(m_executing_command_lists.front());
    return m_executing_command_lists.front();
}

void CommandQueueTracking::CompleteCommandListSetExecution(CommandListSet& executing_command_list_set)
{
    META_FUNCTION_TASK();
    std::unique_lock lock_guard(m_executing_command_lists_mutex);

    if (!m_executing_command_lists.empty() && m_executing_command_lists.front().get() == std::addressof(executing_command_list_set))
    {
        m_executing_command_lists.pop();
    }
}

void CommandQueueTracking::ShutdownQueueExecution()
{
    META_FUNCTION_TASK();
    if (!m_execution_waiting)
        return;

    CompleteExecutionSafely();

    m_execution_waiting_condition_var.notify_one();
    m_execution_waiting_thread.join();
}

void CommandQueueTracking::CompleteExecutionSafely()
{
    META_FUNCTION_TASK();
    std::unique_lock lock(m_execution_waiting_mutex);
    m_timestamp_query_pool_ptr.reset();

    try
    {
        // Do not use virtual call in destructor
        CommandQueueTracking::CompleteExecution();
    }
    catch (const std::exception& ex) // NOSONAR
    {
        META_UNUSED(ex);
        META_LOG("WARNING: Command queue '{}' has failed to complete command list execution, exception occurred: {}", GetName(), ex.what());
        assert(false);
    }

    m_execution_waiting = false;
}

} // namespace Methane::Graphics::Base
