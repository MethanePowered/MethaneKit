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

#pragma once

#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Memory.hpp>

#include <d3d12.h>

namespace Methane::Graphics
{

struct Buffer;
struct IContextDX;
struct ICommandListDX;
struct IResourceDX;
class  CommandQueueDX;
class  QueryBufferDX;
class  TimestampQueryBufferDX;

class QueryDX : public Query
{
public:
    QueryDX(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range);

    // Query overrides
    void Begin() override;
    void End() override;
    void ResolveData() override;
    Resource::SubResource GetData() override;

protected:
    [[nodiscard]] QueryBufferDX& GetQueryBufferDX() noexcept;

private:
    ID3D12GraphicsCommandList&  m_native_command_list;
    const D3D12_QUERY_TYPE      m_native_query_type;
};

class QueryBufferDX : public QueryBuffer
{
public:
    QueryBufferDX(CommandQueueDX& command_queue, Type type,
                  Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size);

    CommandQueueDX&   GetCommandQueueDX() noexcept;
    const IContextDX& GetContextDX() const noexcept          { return m_context_dx; }
    IResourceDX&      GetResultResourceDX() const noexcept   { return m_result_resource_dx; }
    D3D12_QUERY_TYPE  GetNativeQueryType() const noexcept    { return m_native_query_type; }
    ID3D12QueryHeap&  GetNativeQueryHeap() noexcept          { return m_native_query_heap; }

protected:
    Buffer& GetResultBuffer() noexcept { return *m_result_buffer_ptr; }

private:
    Ptr<Buffer>       m_result_buffer_ptr;
    const IContextDX& m_context_dx;
    IResourceDX&      m_result_resource_dx;
    D3D12_QUERY_TYPE  m_native_query_type;
    ID3D12QueryHeap&  m_native_query_heap;
};

class TimestampQueryDX final
    : public QueryDX
      , public TimestampQuery
{
public:
    TimestampQueryDX(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range);

    // TimestampQuery overrides
    void InsertTimestamp() override;
    void ResolveTimestamp() override;
    Timestamp GetGpuTimestamp() override;
    Timestamp GetCpuNanoseconds() override;

private:
    [[nodiscard]] TimestampQueryBufferDX& GetTimestampQueryBufferDX() noexcept;
};

class TimestampQueryBufferDX final
    : public QueryBufferDX
    , public TimestampQueryBuffer
{
public:
    TimestampQueryBufferDX(CommandQueueDX& command_queue, uint32_t max_timestamps_per_frame);

    // ITimestampQueryBuffer interface
    Ptr<TimestampQuery> CreateTimestampQuery(CommandListBase& command_list) override;
};

} // namespace Methane::Graphics
