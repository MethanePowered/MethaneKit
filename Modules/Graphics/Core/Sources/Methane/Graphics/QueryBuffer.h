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

FILE: Methane/Graphics/QueryBuffer.h
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
class QueryBuffer;
struct Context;

using GpuTimeCalibration = std::pair<Timestamp, TimeDelta>;

class Query
{
public:
    using Index = Data::Index;
    using Count = Data::Size;
    using Range = Data::Range<Data::Index>;

    enum class State
    {
        Resolved = 0,
        Begun,
        Ended,
    };

    Query(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range);
    Query(const Query&) = delete;
    Query(Query&&) = delete;
    virtual ~Query();

    virtual void Begin();
    virtual void End();
    virtual void ResolveData();
    virtual Resource::SubResource GetData() const = 0;

    [[nodiscard]] Index            GetIndex() const noexcept       { return m_index; }
    [[nodiscard]] const Range&     GetDataRange() const noexcept   { return m_data_range; }
    [[nodiscard]] State            GetState() const noexcept       { return m_state; }
    [[nodiscard]] QueryBuffer&     GetQueryBuffer() const noexcept { return *m_buffer_ptr; }
    [[nodiscard]] CommandListBase& GetCommandList() const noexcept { return m_command_list; }

private:
    Ptr<QueryBuffer> m_buffer_ptr;
    CommandListBase& m_command_list;
    const Index      m_index;
    const Range      m_data_range;
    State            m_state = State::Resolved;
};

class QueryBuffer : public std::enable_shared_from_this<QueryBuffer>
{
public:
    enum class Type
    {
        Timestamp,
    };

    virtual ~QueryBuffer() = default;

    template<typename QueryT>
    [[nodiscard]] Ptr<QueryT> CreateQuery(CommandListBase& command_list)
    {
        META_FUNCTION_TASK();
        const auto [query_index, query_range] = GetCreateQueryArguments();
        return std::make_shared<QueryT>(*this, command_list, query_index, query_range);
    }

    [[nodiscard]] Ptr<QueryBuffer>  GetPtr()                               { return shared_from_this(); }
    [[nodiscard]] Type              GetType() const noexcept               { return m_type; }
    [[nodiscard]] Data::Size        GetBufferSize() const noexcept         { return m_buffer_size; }
    [[nodiscard]] Data::Size        GetQuerySize() const noexcept          { return m_query_size; }
    [[nodiscard]] Query::Count      GetSlotsCountPerQuery() const noexcept { return m_slots_count_per_query; }
    [[nodiscard]] CommandQueueBase& GetCommandQueueBase() noexcept         { return m_command_queue; }
    [[nodiscard]] const Context&    GetContext() const noexcept            { return m_context; }

protected:
    QueryBuffer(CommandQueueBase& command_queue, Type type,
                Query::Count max_query_count, Query::Count slots_count_per_query,
                Data::Size buffer_size, Data::Size query_size);

    friend class Query;
    virtual void ReleaseQuery(const Query& query);

    using CreateQueryArgs = std::tuple<Query::Index, Query::Range>;
    [[nodiscard]] CreateQueryArgs GetCreateQueryArguments();

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    const Type         m_type;
    const Data::Size   m_buffer_size;
    const Data::Size   m_query_size;
    const Query::Count m_slots_count_per_query;
    RangeSet           m_free_indices;
    RangeSet           m_free_data_ranges;
    CommandQueueBase&  m_command_queue;
    const Context&     m_context;
};

struct TimestampQuery
{
    virtual void InsertTimestamp() = 0;
    virtual void ResolveTimestamp() = 0;

    [[nodiscard]] virtual Timestamp GetGpuTimestamp() const = 0;
    [[nodiscard]] virtual Timestamp GetCpuNanoseconds() const = 0;

    virtual ~TimestampQuery() = default;
};

class TimestampQueryBuffer
{
public:
    using TimestampQuery = Methane::Graphics::TimestampQuery;

    [[nodiscard]] static Ptr<TimestampQueryBuffer> Create(CommandQueueBase& command_queue, uint32_t max_timestamps_per_frame);

    [[nodiscard]] virtual Ptr<TimestampQuery> CreateTimestampQuery(CommandListBase& command_list) = 0;

    virtual ~TimestampQueryBuffer() = default;

    Frequency GetGpuFrequency() const noexcept            { return m_gpu_frequency; }
    TimeDelta GetGpuTimeOffset() const noexcept           { return m_gpu_time_calibration.second; }
    TimeDelta GetGpuCalibrationTimestamp() const noexcept { return m_gpu_time_calibration.first; }

protected:
    void SetGpuFrequency(Frequency gpu_frequency);
    void SetGpuTimeCalibration(const GpuTimeCalibration& gpu_time_calibration);

private:
    Frequency          m_gpu_frequency = 0U;
    GpuTimeCalibration m_gpu_time_calibration{};
};

} // namespace Methane::Graphics
