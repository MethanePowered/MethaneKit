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

FILE: Methane/Graphics/DirectX/QueryPool.hpp
DirectX 12 GPU query pool implementation.

******************************************************************************/

#include <Methane/Graphics/DirectX/QueryPool.h>
#include <Methane/Graphics/DirectX/CommandQueue.h>
#include <Methane/Graphics/DirectX/CommandListSet.h>
#include <Methane/Graphics/DirectX/Buffer.h>
#include <Methane/Graphics/DirectX/IContext.h>

#include <Methane/Graphics/Base/QueryPool.h>
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <wrl.h>
#include <directx/d3d12.h>

namespace wrl = Microsoft::WRL;

namespace Methane::Graphics::Rhi
{

static bool CheckCommandQueueSupportsTimestampQueries(DirectX::CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    if (command_queue.GetNativeCommandQueue().GetDesc().Type != D3D12_COMMAND_LIST_TYPE_COPY)
        return true;

    if (D3D12_FEATURE_DATA_D3D12_OPTIONS3 feature_data{};
        SUCCEEDED(command_queue.GetDirectContext().GetDirectDevice().GetNativeDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &feature_data, sizeof(feature_data))))
        return static_cast<bool>(feature_data.CopyQueueTimestampQueriesSupported);

    return true;
}

Ptr<ITimestampQueryPool> Rhi::ITimestampQueryPool::Create(ICommandQueue& command_queue, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return CheckCommandQueueSupportsTimestampQueries(static_cast<DirectX::CommandQueue&>(command_queue))
           ? std::make_shared<DirectX::TimestampQueryPool>(static_cast<DirectX::CommandQueue&>(command_queue), max_timestamps_per_frame)
           : nullptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

static D3D12_QUERY_TYPE GetQueryTypeDx(Rhi::IQueryPool::Type query_pool_type)
{
    META_FUNCTION_TASK();
    switch(query_pool_type) // NOSONAR - do not use if instead of switch
    {
    case Rhi::IQueryPool::Type::Timestamp: return D3D12_QUERY_TYPE_TIMESTAMP;
    //D3D12_QUERY_TYPE_OCCLUSION
    //D3D12_QUERY_TYPE_BINARY_OCCLUSION
    //D3D12_QUERY_TYPE_PIPELINE_STATISTICS
    default: META_UNEXPECTED_ARG_RETURN(query_pool_type, D3D12_QUERY_TYPE_TIMESTAMP);
    }
}

static D3D12_QUERY_HEAP_TYPE GetQueryHeapTypeDx(Rhi::IQueryPool::Type query_pool_type, D3D12_COMMAND_LIST_TYPE d3d_command_list_type)
{
    META_FUNCTION_TASK();
    switch (query_pool_type) // NOSONAR - do not use if instead of switch
    {
    case Rhi::IQueryPool::Type::Timestamp:
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

static Data::Size GetMaxTimestampsCount(const Rhi::IContext& context, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    const uint32_t frames_count = context.GetType() == Rhi::IContext::Type::Render
                                ? dynamic_cast<const Rhi::IRenderContext&>(context).GetSettings().frame_buffers_count
                                : 1U;
    return frames_count * max_timestamps_per_frame;
}

Query::Query(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range)
    : Base::Query(buffer, command_list, index, data_range)
    , m_native_command_list(dynamic_cast<ICommandListDx&>(command_list).GetNativeCommandList())
    , m_native_query_type(GetQueryTypeDx(buffer.GetType()))
{
    META_FUNCTION_TASK();
}

void Query::Begin()
{
    META_FUNCTION_TASK();
    Base::Query::Begin();
    m_native_command_list.BeginQuery(&GetDirectQueryPool().GetNativeQueryHeap(), m_native_query_type, GetIndex());
}

void Query::End()
{
    META_FUNCTION_TASK();
    Base::Query::End();
    m_native_command_list.EndQuery(&GetDirectQueryPool().GetNativeQueryHeap(), m_native_query_type, GetIndex());
}

void Query::ResolveData()
{
    META_FUNCTION_TASK();
    Base::Query::ResolveData();
    QueryPool& query_pool_dx = GetDirectQueryPool();
    m_native_command_list.ResolveQueryData(
        &query_pool_dx.GetNativeQueryHeap(),
        m_native_query_type,
        GetIndex(), query_pool_dx.GetSlotsCountPerQuery(),
        query_pool_dx.GetDirectResultResource().GetNativeResource(),
        GetDataRange().GetStart()
    );
}

Rhi::IResource::SubResource Query::GetData() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetCommandList().GetState(), Base::CommandList::State::Pending, "query data can be retrieved only when command list is in Pending/Completed state");
    META_CHECK_ARG_EQUAL_DESCR(GetState(), Rhi::IQuery::State::Resolved, "query data can not be retrieved for unresolved query");
    return GetDirectQueryPool().GetDirectResultResource().GetData(Rhi::IResource::SubResource::Index(), GetDataRange());
}

QueryPool& Query::GetDirectQueryPool() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<QueryPool&>(GetQueryPool());
}

QueryPool::QueryPool(CommandQueue& command_queue, Type type,
                         Data::Size max_query_count, Rhi::IQuery::Count slots_count_per_query,
                         Data::Size buffer_size, Data::Size query_size)
    : Base::QueryPool(static_cast<Base::CommandQueue&>(command_queue), type, max_query_count, slots_count_per_query, buffer_size, query_size)
    , m_result_buffer_ptr(Rhi::IBuffer::CreateReadBackBuffer(GetContext(), buffer_size))
    , m_context_dx(dynamic_cast<const IContext&>(GetContext()))
    , m_result_resource_dx(dynamic_cast<IResource&>(*m_result_buffer_ptr))
    , m_native_query_type(GetQueryTypeDx(type))
    , m_native_query_heap(m_context_dx.GetNativeQueryHeap(GetQueryHeapTypeDx(type, command_queue.GetNativeCommandQueue().GetDesc().Type), max_query_count))
{
    META_FUNCTION_TASK();
}

CommandQueue& QueryPool::GetDirectCommandQueue() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetBaseCommandQueue());
}

TimestampQueryPool::TimestampQueryPool(CommandQueue& command_queue, uint32_t max_timestamps_per_frame)
    : QueryPool(command_queue, Type::Timestamp, 1U << 15U, 1U,
                  GetMaxTimestampsCount(command_queue.GetContext(), max_timestamps_per_frame) * sizeof(Timestamp),
                  sizeof(Timestamp))
{
    META_FUNCTION_TASK();
    SetGpuFrequency(DirectX::GetGpuFrequency(GetDirectCommandQueue().GetNativeCommandQueue(), *GetDirectContext().GetDirectDevice().GetNativeDevice().Get()));
    Calibrate();
}

Ptr<Rhi::ITimestampQuery> TimestampQueryPool::CreateTimestampQuery(Rhi::ICommandList& command_list)
{
    META_FUNCTION_TASK();
    return Base::QueryPool::CreateQuery<TimestampQuery>(dynamic_cast<Base::CommandList&>(command_list));
}

Rhi::ITimestampQueryPool::CalibratedTimestamps TimestampQueryPool::Calibrate()
{
    META_FUNCTION_TASK();
    CalibratedTimestamps calibrated_timestamps{ 0U, 0U };
    ThrowIfFailed(GetDirectCommandQueue().GetNativeCommandQueue().GetClockCalibration(&calibrated_timestamps.gpu_ts, &calibrated_timestamps.cpu_ts),
                  GetDirectContext().GetDirectDevice().GetNativeDevice().Get());
    calibrated_timestamps.cpu_ts *= Data::GetQpcToNSecMultiplier();
    SetCalibratedTimestamps(calibrated_timestamps);
    return calibrated_timestamps;
}

TimestampQuery::TimestampQuery(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range)
    : Query(buffer, command_list, index, data_range)
{
    META_FUNCTION_TASK();
}

void TimestampQuery::InsertTimestamp()
{
    META_FUNCTION_TASK();
    Query::End();
}

void TimestampQuery::ResolveTimestamp()
{
    META_FUNCTION_TASK();
    Query::ResolveData();
}

Timestamp TimestampQuery::GetGpuTimestamp() const
{
    META_FUNCTION_TASK();
    Rhi::IResource::SubResource query_data = GetData();
    META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(query_data.GetDataSize(), sizeof(Timestamp), "query data size is less than expected for timestamp");
    META_CHECK_ARG_NOT_NULL(query_data.GetDataPtr());
    return *reinterpret_cast<const Timestamp*>(query_data.GetDataPtr()); // NOSONAR
}

Timestamp TimestampQuery::GetCpuNanoseconds() const
{
    META_FUNCTION_TASK();
    const TimestampQueryPool& timestamp_query_pool_dx = GetDirectTimestampQueryPool();
    const Timestamp gpu_timestamp = TimestampQuery::GetGpuTimestamp();
    return Data::ConvertTicksToNanoseconds(gpu_timestamp - timestamp_query_pool_dx.GetGpuTimeOffset(), timestamp_query_pool_dx.GetGpuFrequency());
}

TimestampQueryPool& TimestampQuery::GetDirectTimestampQueryPool() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<TimestampQueryPool&>(GetQueryPool());
}

} // namespace Methane::Graphics::DirectX
