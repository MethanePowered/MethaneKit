/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/QueryBufferDX.hpp
DirectX 12 GPU query results buffer.

******************************************************************************/

#include "QueryBufferDX.h"
#include "CommandQueueDX.h"
#include "CommandListDX.h"
#include "BufferDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <d3d12.h>

namespace wrl = Microsoft::WRL;

namespace Methane::Graphics
{

static D3D12_QUERY_TYPE GetQueryTypeDx(QueryBuffer::Type query_buffer_type)
{
    META_FUNCTION_TASK();
    switch(query_buffer_type)
    {
    case QueryBuffer::Type::Timestamp: return D3D12_QUERY_TYPE_TIMESTAMP;
    //D3D12_QUERY_TYPE_OCCLUSION
    //D3D12_QUERY_TYPE_BINARY_OCCLUSION
    //D3D12_QUERY_TYPE_PIPELINE_STATISTICS
    default: META_UNEXPECTED_ARG_RETURN(query_buffer_type, D3D12_QUERY_TYPE_TIMESTAMP);
    }
}

static D3D12_QUERY_HEAP_TYPE GetQueryHeapTypeDx(QueryBuffer::Type query_buffer_type, D3D12_COMMAND_LIST_TYPE d3d_command_list_type)
{
    META_FUNCTION_TASK();
    switch (query_buffer_type)
    {
    case QueryBuffer::Type::Timestamp:
        return d3d_command_list_type == D3D12_COMMAND_LIST_TYPE_COPY
             ? D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP
             : D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    //D3D12_QUERY_HEAP_TYPE_OCCLUSION
    //D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS
    default:
        META_UNEXPECTED_ARG_RETURN(query_buffer_type, D3D12_QUERY_HEAP_TYPE_TIMESTAMP);
    }
}

static Frequency GetGpuFrequency(ID3D12CommandQueue& native_command_queue, ID3D12Device& native_device)
{
    META_FUNCTION_TASK();
    Frequency gpu_frequency = 0U;
    ThrowIfFailed(native_command_queue.GetTimestampFrequency(&gpu_frequency), &native_device);
    return gpu_frequency;
}

static GpuTimeCalibration GetGpuTimeCalibration(ID3D12CommandQueue& native_command_queue, ID3D12Device& native_device)
{
    META_FUNCTION_TASK();
    UINT64 gpu_timestamp = 0U;
    UINT64 cpu_timestamp = 0U;
    ThrowIfFailed(native_command_queue.GetClockCalibration(&gpu_timestamp, &cpu_timestamp), &native_device);
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

QueryDX::QueryDX(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range)
    : Query(buffer, command_list, index, data_range)
    , m_native_command_list(dynamic_cast<ICommandListDX&>(command_list).GetNativeCommandList())
    , m_native_query_type(GetQueryTypeDx(buffer.GetType()))
{
    META_FUNCTION_TASK();
}

void QueryDX::Begin()
{
    META_FUNCTION_TASK();
    Query::Begin();
    m_native_command_list.BeginQuery(&GetQueryBufferDX().GetNativeQueryHeap(), m_native_query_type, GetIndex());
}

void QueryDX::End()
{
    META_FUNCTION_TASK();
    Query::End();
    m_native_command_list.EndQuery(&GetQueryBufferDX().GetNativeQueryHeap(), m_native_query_type, GetIndex());
}

void QueryDX::ResolveData()
{
    META_FUNCTION_TASK();
    Query::ResolveData();
    QueryBufferDX& query_buffer_dx = GetQueryBufferDX();
    m_native_command_list.ResolveQueryData(
        &query_buffer_dx.GetNativeQueryHeap(),
        m_native_query_type,
        GetIndex(), query_buffer_dx.GetSlotsCountPerQuery(),
        query_buffer_dx.GetResultResourceDX().GetNativeResource(),
        GetDataRange().GetStart()
    );
}

Resource::SubResource QueryDX::GetData() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetCommandList().GetState(), CommandListBase::State::Pending, "query data can be retrieved only when command list is in Pending/Completed state");
    // FIXME: META_CHECK_ARG_EQUAL_DESCR(GetState(), Query::State::Resolved, "query data can not be retrieved for unresolved query")
    assert(GetState() == Query::State::Resolved);
    return GetQueryBufferDX().GetResultResourceDX().GetData(Resource::SubResource::Index(), GetDataRange());
}

QueryBufferDX& QueryDX::GetQueryBufferDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<QueryBufferDX&>(GetQueryBuffer());
}

Ptr<TimestampQueryBuffer> TimestampQueryBuffer::Create(CommandQueueBase& command_queue, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return std::make_shared<TimestampQueryBufferDX>(static_cast<CommandQueueDX&>(command_queue), max_timestamps_per_frame);
}

QueryBufferDX::QueryBufferDX(CommandQueueDX& command_queue, Type type,
                             Data::Size max_query_count, Query::Count slots_count_per_query,
                             Data::Size buffer_size, Data::Size query_size)
    : QueryBuffer(static_cast<CommandQueueBase&>(command_queue), type, max_query_count, slots_count_per_query, buffer_size, query_size)
    , m_result_buffer_ptr(Buffer::CreateReadBackBuffer(GetContext(), buffer_size))
    , m_context_dx(dynamic_cast<const IContextDX&>(GetContext()))
    , m_result_resource_dx(dynamic_cast<IResourceDX&>(*m_result_buffer_ptr))
    , m_native_query_type(GetQueryTypeDx(type))
    , m_native_query_heap(m_context_dx.GetNativeQueryHeap(GetQueryHeapTypeDx(type, command_queue.GetNativeCommandQueue().GetDesc().Type), max_query_count))
{
    META_FUNCTION_TASK();
}

CommandQueueDX& QueryBufferDX::GetCommandQueueDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetCommandQueueBase());
}

TimestampQueryBufferDX::TimestampQueryBufferDX(CommandQueueDX& command_queue, uint32_t max_timestamps_per_frame)
    : QueryBufferDX(command_queue, Type::Timestamp, 1U << 15U, 1U,
                    GetMaxTimestampsCount(command_queue.GetContext(), max_timestamps_per_frame) * sizeof(Timestamp),
                    sizeof(Timestamp))
{
    META_FUNCTION_TASK();
    SetGpuFrequency(Graphics::GetGpuFrequency(GetCommandQueueDX().GetNativeCommandQueue(), *GetContextDX().GetDeviceDX().GetNativeDevice().Get()));
    SetGpuTimeCalibration(Graphics::GetGpuTimeCalibration(GetCommandQueueDX().GetNativeCommandQueue(), *GetContextDX().GetDeviceDX().GetNativeDevice().Get()));
}

Ptr<TimestampQuery> TimestampQueryBufferDX::CreateTimestampQuery(CommandListBase& command_list)
{
    META_FUNCTION_TASK();
    return QueryBuffer::CreateQuery<TimestampQueryDX>(command_list);
}

TimestampQueryDX::TimestampQueryDX(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range)
    : QueryDX(buffer, command_list, index, data_range)
{
    META_FUNCTION_TASK();
}

void TimestampQueryDX::InsertTimestamp()
{
    META_FUNCTION_TASK();
    QueryDX::End();
}

void TimestampQueryDX::ResolveTimestamp()
{
    META_FUNCTION_TASK();
    QueryDX::ResolveData();
}

Timestamp TimestampQueryDX::GetGpuTimestamp() const
{
    META_FUNCTION_TASK();
    Resource::SubResource query_data = GetData();
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(query_data.GetDataSize(), sizeof(Timestamp), "query data size is less than expected for timestamp");
    META_CHECK_ARG_NOT_NULL(query_data.GetDataPtr());
    return *reinterpret_cast<const Timestamp*>(query_data.GetDataPtr()); // NOSONAR
}

Timestamp TimestampQueryDX::GetCpuNanoseconds() const
{
    META_FUNCTION_TASK();
    const TimestampQueryBufferDX& timestamp_query_buffer_dx = GetTimestampQueryBufferDX();
    const Timestamp gpu_timestamp = TimestampQueryDX::GetGpuTimestamp();
    return Data::ConvertTicksToNanoseconds(gpu_timestamp - timestamp_query_buffer_dx.GetGpuTimeOffset(), timestamp_query_buffer_dx.GetGpuFrequency());
}

TimestampQueryBufferDX& TimestampQueryDX::GetTimestampQueryBufferDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<TimestampQueryBufferDX&>(GetQueryBuffer());
}

} // namespace Methane::Graphics
