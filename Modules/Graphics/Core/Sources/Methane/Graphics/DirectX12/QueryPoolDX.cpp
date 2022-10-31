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

FILE: Methane/Graphics/DirectX12/QueryPoolDX.hpp
DirectX 12 GPU query pool implementation.

******************************************************************************/

#include "QueryPoolDX.h"
#include "CommandQueueDX.h"
#include "CommandListDX.h"
#include "BufferDX.h"
#include "ContextDX.h"

#include <Methane/Graphics/QueryPoolBase.h>
#include <Methane/Graphics/IRenderContext.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <directx/d3d12.h>

namespace wrl = Microsoft::WRL;

namespace Methane::Graphics
{

static D3D12_QUERY_TYPE GetQueryTypeDx(IQueryPool::Type query_pool_type)
{
    META_FUNCTION_TASK();
    switch(query_pool_type) // NOSONAR - do not use if instead of switch
    {
    case IQueryPool::Type::Timestamp: return D3D12_QUERY_TYPE_TIMESTAMP;
    //D3D12_QUERY_TYPE_OCCLUSION
    //D3D12_QUERY_TYPE_BINARY_OCCLUSION
    //D3D12_QUERY_TYPE_PIPELINE_STATISTICS
    default: META_UNEXPECTED_ARG_RETURN(query_pool_type, D3D12_QUERY_TYPE_TIMESTAMP);
    }
}

static D3D12_QUERY_HEAP_TYPE GetQueryHeapTypeDx(IQueryPool::Type query_pool_type, D3D12_COMMAND_LIST_TYPE d3d_command_list_type)
{
    META_FUNCTION_TASK();
    switch (query_pool_type) // NOSONAR - do not use if instead of switch
    {
    case IQueryPool::Type::Timestamp:
        return d3d_command_list_type == D3D12_COMMAND_LIST_TYPE_COPY
             ? D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP
             : D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    //D3D12_QUERY_HEAP_TYPE_OCCLUSION
    //D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS
    default:
        META_UNEXPECTED_ARG_RETURN(query_pool_type, D3D12_QUERY_HEAP_TYPE_TIMESTAMP);
    }
}

static Frequency GetGpuFrequency(ID3D12CommandQueue& native_command_queue, ID3D12Device& native_device)
{
    META_FUNCTION_TASK();
    Frequency gpu_frequency = 0U;
    ThrowIfFailed(native_command_queue.GetTimestampFrequency(&gpu_frequency), &native_device);
    return gpu_frequency;
}

static Data::Size GetMaxTimestampsCount(const IContext& context, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    const uint32_t frames_count = context.GetType() == IContext::Type::Render
                                  ? dynamic_cast<const IRenderContext&>(context).GetSettings().frame_buffers_count
                                  : 1U;
    return frames_count * max_timestamps_per_frame;
}

static bool CheckCommandQueueSupportsTimestampQueries(CommandQueueDX& command_queue)
{
    META_FUNCTION_TASK();
    if (command_queue.GetNativeCommandQueue().GetDesc().Type != D3D12_COMMAND_LIST_TYPE_COPY)
        return true;

    if (D3D12_FEATURE_DATA_D3D12_OPTIONS3 feature_data{};
        SUCCEEDED(command_queue.GetContextDX().GetDeviceDX().GetNativeDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &feature_data, sizeof(feature_data))))
        return static_cast<bool>(feature_data.CopyQueueTimestampQueriesSupported);

    return true;
}

QueryDX::QueryDX(QueryPoolBase& buffer, CommandListBase& command_list, Index index, Range data_range)
    : QueryBase(buffer, command_list, index, data_range)
    , m_native_command_list(dynamic_cast<ICommandListDX&>(command_list).GetNativeCommandList())
    , m_native_query_type(GetQueryTypeDx(buffer.GetType()))
{
    META_FUNCTION_TASK();
}

void QueryDX::Begin()
{
    META_FUNCTION_TASK();
    QueryBase::Begin();
    m_native_command_list.BeginQuery(&GetQueryPoolDX().GetNativeQueryHeap(), m_native_query_type, GetIndex());
}

void QueryDX::End()
{
    META_FUNCTION_TASK();
    QueryBase::End();
    m_native_command_list.EndQuery(&GetQueryPoolDX().GetNativeQueryHeap(), m_native_query_type, GetIndex());
}

void QueryDX::ResolveData()
{
    META_FUNCTION_TASK();
    QueryBase::ResolveData();
    QueryPoolDX& query_pool_dx = GetQueryPoolDX();
    m_native_command_list.ResolveQueryData(
        &query_pool_dx.GetNativeQueryHeap(),
        m_native_query_type,
        GetIndex(), query_pool_dx.GetSlotsCountPerQuery(),
        query_pool_dx.GetResultResourceDX().GetNativeResource(),
        GetDataRange().GetStart()
    );
}

IResource::SubResource QueryDX::GetData() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetCommandList().GetState(), CommandListBase::State::Pending, "query data can be retrieved only when command list is in Pending/Completed state");
    META_CHECK_ARG_EQUAL_DESCR(GetState(), IQuery::State::Resolved, "query data can not be retrieved for unresolved query");
    return GetQueryPoolDX().GetResultResourceDX().GetData(IResource::SubResource::Index(), GetDataRange());
}

QueryPoolDX& QueryDX::GetQueryPoolDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<QueryPoolDX&>(GetQueryPool());
}

Ptr<ITimestampQueryPool> ITimestampQueryPool::Create(ICommandQueue& command_queue, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return CheckCommandQueueSupportsTimestampQueries(static_cast<CommandQueueDX&>(command_queue))
         ? std::make_shared<TimestampQueryPoolDX>(static_cast<CommandQueueDX&>(command_queue), max_timestamps_per_frame)
         : nullptr;
}

QueryPoolDX::QueryPoolDX(CommandQueueDX& command_queue, Type type,
                         Data::Size max_query_count, IQuery::Count slots_count_per_query,
                         Data::Size buffer_size, Data::Size query_size)
    : QueryPoolBase(static_cast<CommandQueueBase&>(command_queue), type, max_query_count, slots_count_per_query, buffer_size, query_size)
    , m_result_buffer_ptr(IBuffer::CreateReadBackBuffer(GetContext(), buffer_size))
    , m_context_dx(dynamic_cast<const IContextDX&>(GetContext()))
    , m_result_resource_dx(dynamic_cast<IResourceDX&>(*m_result_buffer_ptr))
    , m_native_query_type(GetQueryTypeDx(type))
    , m_native_query_heap(m_context_dx.GetNativeQueryHeap(GetQueryHeapTypeDx(type, command_queue.GetNativeCommandQueue().GetDesc().Type), max_query_count))
{
    META_FUNCTION_TASK();
}

CommandQueueDX& QueryPoolDX::GetCommandQueueDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetCommandQueueBase());
}

TimestampQueryPoolDX::TimestampQueryPoolDX(CommandQueueDX& command_queue, uint32_t max_timestamps_per_frame)
    : QueryPoolDX(command_queue, Type::Timestamp, 1U << 15U, 1U,
                  GetMaxTimestampsCount(command_queue.GetContext(), max_timestamps_per_frame) * sizeof(Timestamp),
                  sizeof(Timestamp))
{
    META_FUNCTION_TASK();
    SetGpuFrequency(Graphics::GetGpuFrequency(GetCommandQueueDX().GetNativeCommandQueue(), *GetContextDX().GetDeviceDX().GetNativeDevice().Get()));
    Calibrate();
}

Ptr<ITimestampQuery> TimestampQueryPoolDX::CreateTimestampQuery(ICommandList& command_list)
{
    META_FUNCTION_TASK();
    return QueryPoolBase::CreateQuery<TimestampQueryDX>(dynamic_cast<CommandListBase&>(command_list));
}

ITimestampQueryPool::CalibratedTimestamps TimestampQueryPoolDX::Calibrate()
{
    META_FUNCTION_TASK();
    CalibratedTimestamps calibrated_timestamps{ 0U, 0U };
    ThrowIfFailed(GetCommandQueueDX().GetNativeCommandQueue().GetClockCalibration(&calibrated_timestamps.gpu_ts, &calibrated_timestamps.cpu_ts),
                  GetContextDX().GetDeviceDX().GetNativeDevice().Get());
    calibrated_timestamps.cpu_ts *= Data::GetQpcToNSecMultiplier();
    SetCalibratedTimestamps(calibrated_timestamps);
    return calibrated_timestamps;
}

TimestampQueryDX::TimestampQueryDX(QueryPoolBase& buffer, CommandListBase& command_list, Index index, Range data_range)
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
    IResource::SubResource query_data = GetData();
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(query_data.GetDataSize(), sizeof(Timestamp), "query data size is less than expected for timestamp");
    META_CHECK_ARG_NOT_NULL(query_data.GetDataPtr());
    return *reinterpret_cast<const Timestamp*>(query_data.GetDataPtr()); // NOSONAR
}

Timestamp TimestampQueryDX::GetCpuNanoseconds() const
{
    META_FUNCTION_TASK();
    const TimestampQueryPoolDX& timestamp_query_pool_dx = GetTimestampQueryPoolDX();
    const Timestamp gpu_timestamp = TimestampQueryDX::GetGpuTimestamp();
    return Data::ConvertTicksToNanoseconds(gpu_timestamp - timestamp_query_pool_dx.GetGpuTimeOffset(), timestamp_query_pool_dx.GetGpuFrequency());
}

TimestampQueryPoolDX& TimestampQueryDX::GetTimestampQueryPoolDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<TimestampQueryPoolDX&>(GetQueryPool());
}

} // namespace Methane::Graphics
