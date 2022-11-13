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

#pragma once

#include <Methane/Graphics/Base/QueryPool.h>
#include <Methane/Memory.hpp>

#include <directx/d3d12.h>

namespace Methane::Graphics::Rhi
{

struct IBuffer;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

struct IContextDx;
struct ICommandListDx;
struct IResourceDx;
class  CommandQueue;
class  QueryPool;
class  TimestampQueryPool;

class Query : public Base::Query
{
public:
    Query(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // Query overrides
    void Begin() override;
    void End() override;
    void ResolveData() override;
    Rhi::SubResource GetData() const override;

protected:
    [[nodiscard]] QueryPool& GetDirectQueryPool() const noexcept;

private:
    ID3D12GraphicsCommandList&  m_native_command_list;
    const D3D12_QUERY_TYPE      m_native_query_type;
};

class QueryPool : public Base::QueryPool
{
public:
    QueryPool(CommandQueue& command_queue, Type type,
                Data::Size max_query_count, Rhi::IQuery::Count slots_count_per_query,
                Data::Size buffer_size, Data::Size query_size);

    CommandQueue&      GetDirectCommandQueue() noexcept;
    const IContextDx&  GetDirectContext() const noexcept        { return m_context_dx; }
    IResourceDx&       GetDirectResultResource() const noexcept { return m_result_resource_dx; }
    D3D12_QUERY_TYPE   GetNativeQueryType() const noexcept      { return m_native_query_type; }
    ID3D12QueryHeap&   GetNativeQueryHeap() noexcept            { return m_native_query_heap; }

protected:
    IBuffer& GetResultBuffer() noexcept { return *m_result_buffer_ptr; }

private:
    Ptr<IBuffer>      m_result_buffer_ptr;
    const IContextDx& m_context_dx;
    IResourceDx&      m_result_resource_dx;
    D3D12_QUERY_TYPE  m_native_query_type;
    ID3D12QueryHeap&  m_native_query_heap;
};

class TimestampQuery final
    : protected Query
    , public ITimestampQuery
{
public:
    TimestampQuery(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // TimestampQuery overrides
    void InsertTimestamp() override;
    void ResolveTimestamp() override;
    Timestamp GetGpuTimestamp() const override;
    Timestamp GetCpuNanoseconds() const override;

private:
    [[nodiscard]] TimestampQueryPool& GetDirectTimestampQueryPool() const noexcept;
};

class TimestampQueryPool final
    : public QueryPool
    , public Base::TimestampQueryPool
{
public:
    TimestampQueryPool(CommandQueue& command_queue, uint32_t max_timestamps_per_frame);

    // ITimestampQueryPool interface
    Ptr<ITimestampQuery> CreateTimestampQuery(ICommandList& command_list) override;
    CalibratedTimestamps Calibrate() override;
};

} // namespace Methane::Graphics::DirectX
