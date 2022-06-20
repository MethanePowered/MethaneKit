/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/QueryBufferVK.hpp
Vulkan GPU query results buffer.

******************************************************************************/

#include "QueryBufferVK.h"
#include "CommandQueueVK.h"
#include "CommandListVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

static vk::QueryType GetQueryTypeVk(QueryBuffer::Type query_buffer_type)
{
    META_FUNCTION_TASK();
    switch(query_buffer_type)
    {
    case QueryBuffer::Type::Timestamp: return vk::QueryType::eTimestamp;
    // vk::QueryType::eOcclusion
    // vk::QueryType::ePipelineStatistics
    default: META_UNEXPECTED_ARG_RETURN(query_buffer_type, vk::QueryType::eTimestamp);
    }
}

static Frequency GetGpuFrequency()
{
    META_FUNCTION_TASK();
    Frequency gpu_frequency = 0U;
    return gpu_frequency;
}

static GpuTimeCalibration GetGpuTimeCalibration()
{
    META_FUNCTION_TASK();
    uint64_t gpu_timestamp = 0U;
    uint64_t cpu_timestamp = 0U;
    return { gpu_timestamp, static_cast<TimeDelta>(gpu_timestamp - cpu_timestamp) };
}

static Data::Size GetMaxTimestampsCount(const Context& context, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    const uint32_t frames_count = context.GetType() == Context::Type::Render
                                  ? dynamic_cast<const RenderContext&>(context).GetSettings().frame_buffers_count
                                  : 1U;
    return frames_count * max_timestamps_per_frame;
}

QueryVK::QueryVK(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range)
    : Query(buffer, command_list, index, data_range)
{
    META_FUNCTION_TASK();
}

void QueryVK::Begin()
{
    META_FUNCTION_TASK();
    Query::Begin();
}

void QueryVK::End()
{
    META_FUNCTION_TASK();
    Query::End();
}

void QueryVK::ResolveData()
{
    META_FUNCTION_TASK();
    Query::ResolveData();
    //QueryBufferVK& query_buffer_dx = GetQueryBufferVK();
}

SubResource QueryVK::GetData()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetCommandList().GetState(), CommandListBase::State::Pending, "query data can be retrieved only when command list is in Pending/Completed state");
    META_CHECK_ARG_EQUAL_DESCR(GetState(), Query::State::Resolved, "query data can not be retrieved for unresolved query");
    return SubResource(nullptr, 0U);
}

QueryBufferVK& QueryVK::GetQueryBufferVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<QueryBufferVK&>(GetQueryBuffer());
}

Ptr<TimestampQueryBuffer> TimestampQueryBuffer::Create(CommandQueueBase& command_queue, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return std::make_shared<TimestampQueryBufferVK>(static_cast<CommandQueueVK&>(command_queue), max_timestamps_per_frame);
}

QueryBufferVK::QueryBufferVK(CommandQueueVK& command_queue, Type type, Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size)
    : QueryBuffer(static_cast<CommandQueueBase&>(command_queue), type, max_query_count, buffer_size, query_size)
    , m_context_vk(dynamic_cast<const IContextVK&>(GetContext()))
    , m_vk_query_pool(command_queue.GetDeviceVK().GetNativeDevice().createQueryPool(vk::QueryPoolCreateInfo({}, GetQueryTypeVk(type), max_query_count)))
{
    META_FUNCTION_TASK();
}

CommandQueueVK& QueryBufferVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(GetCommandQueueBase());
}

TimestampQueryBufferVK::TimestampQueryBufferVK(CommandQueueVK& command_queue, uint32_t max_timestamps_per_frame)
    : QueryBufferVK(command_queue, Type::Timestamp, 1U << 15U,
                    GetMaxTimestampsCount(command_queue.GetContext(), max_timestamps_per_frame) * sizeof(Timestamp),
                    sizeof(Timestamp))
{
    META_FUNCTION_TASK();
    SetGpuFrequency(Graphics::GetGpuFrequency());
    SetCpuTimeCalibration(Graphics::GetGpuTimeCalibration());
}

Ptr<TimestampQueryBuffer::TimestampQuery> TimestampQueryBufferVK::CreateTimestampQuery(CommandListBase& command_list)
{
    META_FUNCTION_TASK();
    return QueryBuffer::CreateQuery<TimestampQueryVK>(command_list);
}

TimestampQueryVK::TimestampQueryVK(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range)
    : QueryVK(buffer, command_list, index, data_range)
{
    META_FUNCTION_TASK();
}

void TimestampQueryVK::InsertTimestamp()
{
    META_FUNCTION_TASK();
    QueryVK::End();
}

void TimestampQueryVK::ResolveTimestamp()
{
    META_FUNCTION_TASK();
    QueryVK::ResolveData();
}

Timestamp TimestampQueryVK::GetGpuTimestamp()
{
    META_FUNCTION_TASK();
    Resource::SubResource query_data = GetData();
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(query_data.GetDataSize(), sizeof(Timestamp), "query data size is less than expected for timestamp");
    META_CHECK_ARG_NOT_NULL(query_data.GetDataPtr());
    return *reinterpret_cast<const Timestamp*>(query_data.GetDataPtr()); // NOSONAR
}

Timestamp TimestampQueryVK::GetCpuNanoseconds()
{
    META_FUNCTION_TASK();
    const TimestampQueryBufferVK& timestamp_query_buffer_dx = GetTimestampQueryBufferVK();
    const Timestamp gpu_timestamp = TimestampQueryVK::GetGpuTimestamp();
    return Data::ConvertTicksToNanoseconds(gpu_timestamp - timestamp_query_buffer_dx.GetGpuTimeOffset(), timestamp_query_buffer_dx.GetGpuFrequency());
}

TimestampQueryBufferVK& TimestampQueryVK::GetTimestampQueryBufferVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<TimestampQueryBufferVK&>(GetQueryBuffer());
}

} // namespace Methane::Graphics
