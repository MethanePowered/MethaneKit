/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/QueryBuffer.h
GPU data query buffer base implementation.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics
{

class CommandQueueBase;
struct Context;

class QueryBuffer
{
public:
    enum class Type
    {
        Timestamp,
    };

    class Query
    {
    public:
        using Index = Data::Index;
        using Range = Data::Range<Data::Index>;

        Query(QueryBuffer& buffer, Index index, Range data_range);
        virtual ~Query();

        Index        GetIndex() const noexcept      { return m_index; }
        const Range& GetDataRange() const noexcept  { return m_data_range; }

    private:
        QueryBuffer& m_buffer;
        const Index  m_index;
        const Range  m_data_range;
    };

    QueryBuffer(CommandQueueBase& command_queue, Type type,
                Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size);
    virtual ~QueryBuffer() = default;

    virtual Ptr<Query> CreateQuery();

    Type              GetType() const noexcept       { return m_type; }
    Data::Size        GetBufferSize() const noexcept { return m_buffer_size; }
    Data::Size        GetQuerySize() const noexcept  { return m_query_size; }
    CommandQueueBase& GetCommandQueueBase() noexcept { return m_command_queue; }
    Context&          GetContext() noexcept          { return m_context; }

protected:
    friend class Query;
    virtual void ReleaseQuery(const Query& query);

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    const Type        m_type;
    const Data::Size  m_buffer_size;
    const Data::Size  m_query_size;
    RangeSet          m_free_indices;
    RangeSet          m_free_data_ranges;
    CommandQueueBase& m_command_queue;
    Context&          m_context;
};

using GpuTimestamp = uint64_t;
using GpuFrequency = uint64_t;

struct ITimestampQueryBuffer
{
    static Data::Size GetTimestampResultBufferSize(const Context& context, uint32_t max_timestamps_per_frame);

    virtual GpuFrequency GetGpuFrequency() const noexcept = 0;

    virtual ~ITimestampQueryBuffer() = default;
};

} // namespace Methane::Graphics
