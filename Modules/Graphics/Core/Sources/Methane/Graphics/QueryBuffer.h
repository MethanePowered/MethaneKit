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

#include <Methane/Graphics/Resource.h>
#include <Methane/Data/Types.h>
#include <Methane/Data/TimeRange.hpp>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics
{

class CommandQueueBase;
class CommandListBase;
struct Context;

class QueryBuffer : public std::enable_shared_from_this<QueryBuffer>
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

        enum class State
        {
            Resolved = 0,
            Begun,
            Ended,
        };

        Query(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range);
        virtual ~Query();

        virtual void Begin();
        virtual void End();
        virtual void ResolveData();
        virtual Resource::SubResource GetData() = 0;

        Index            GetIndex() const noexcept       { return m_index; }
        const Range&     GetDataRange() const noexcept   { return m_data_range; }
        State            GetState() const noexcept       { return m_state; }
        QueryBuffer&     GetQueryBuffer() const noexcept { return *m_sp_buffer; }
        CommandListBase& GetCommandList() const noexcept { return m_command_list; }

    private:
        Ptr<QueryBuffer> m_sp_buffer;
        CommandListBase& m_command_list;
        const Index      m_index;
        const Range      m_data_range;
        State            m_state = State::Resolved;
    };

    virtual ~QueryBuffer() = default;

    template<typename QueryT>
    Ptr<QueryT> CreateQuery(CommandListBase& command_list)
    {
        META_FUNCTION_TASK();
        const CreateQueryArgs   query_args = GetCreateQueryArguments();
        return std::make_shared<QueryT>(*this, command_list, std::get<0>(query_args), std::get<1>(query_args));
    }

    Ptr<QueryBuffer>  GetPtr()                       { return shared_from_this(); }
    Type              GetType() const noexcept       { return m_type; }
    Data::Size        GetBufferSize() const noexcept { return m_buffer_size; }
    Data::Size        GetQuerySize() const noexcept  { return m_query_size; }
    CommandQueueBase& GetCommandQueueBase() noexcept { return m_command_queue; }
    Context&          GetContext() noexcept          { return m_context; }

    static std::string GetTypeName(Type type) noexcept;

protected:
    QueryBuffer(CommandQueueBase& command_queue, Type type,
                Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size);

    friend class Query;
    virtual void ReleaseQuery(const Query& query);

    using CreateQueryArgs = std::tuple<Query::Index, Query::Range>;
    CreateQueryArgs GetCreateQueryArguments();

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

using GpuFrequency = uint64_t;

struct TimestampQueryBuffer
{
    struct TimestampQuery
    {
        virtual void InsertTimestamp() = 0;
        virtual void ResolveTimestamp() = 0;
        virtual Timestamp GetTimestamp() = 0;

        virtual ~TimestampQuery() = default;
    };

    static Ptr<TimestampQueryBuffer> Create(CommandQueueBase& command_queue, uint32_t max_timestamps_per_frame);

    virtual Ptr<TimestampQuery> CreateTimestampQuery(CommandListBase& command_list) = 0;
    virtual GpuFrequency GetGpuFrequency() const noexcept = 0;

    virtual ~TimestampQueryBuffer() = default;
};

} // namespace Methane::Graphics
