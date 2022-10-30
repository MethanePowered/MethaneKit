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

FILE: Methane/Graphics/QueryPoolBase.h
GPU data query pool base implementation.

******************************************************************************/

#pragma once

#include "QueryPool.h"

#include <Methane/Graphics/IResource.h>
#include <Methane/Data/Types.h>
#include <Methane/Data/TimeRange.hpp>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics
{

class CommandQueueBase;
class CommandListBase;
class QueryPoolBase;

class QueryBase // NOSONAR - custom destructor is required
    : public IQuery
{
public:
    QueryBase(QueryPoolBase& buffer, CommandListBase& command_list, Index index, Range data_range);
    QueryBase(const QueryBase&) = delete;
    QueryBase(QueryBase&&) = delete;
    ~QueryBase() override;

    // IQuery overrides
    void Begin() override;
    void End() override;
    void ResolveData() override;

    [[nodiscard]] Index        GetIndex() const noexcept final       { return m_index; }
    [[nodiscard]] const Range& GetDataRange() const noexcept final   { return m_data_range; }
    [[nodiscard]] State        GetState() const noexcept final       { return m_state; }
    [[nodiscard]] IQueryPool&  GetQueryPool() const noexcept final;
    [[nodiscard]] CommandList& GetCommandList() const noexcept final;

private:
    Ptr<QueryPoolBase> m_query_pool_ptr;
    CommandListBase&   m_command_list;
    const Index        m_index;
    const Range        m_data_range;
    State              m_state = State::Resolved;
};

class QueryPoolBase
    : public IQueryPool
    , public std::enable_shared_from_this<QueryPoolBase>
{
public:
    template<typename QueryT>
    [[nodiscard]] Ptr<QueryT> CreateQuery(CommandListBase& command_list)
    {
        const auto [query_index, query_range] = GetCreateQueryArguments();
        return std::make_shared<QueryT>(*this, command_list, query_index, query_range);
    }

    // IQueryPool overrides
    [[nodiscard]] Ptr<IQueryPool> GetPtr() final                               { return std::dynamic_pointer_cast<IQueryPool>(shared_from_this()); }
    [[nodiscard]] Type            GetType() const noexcept final               { return m_type; }
    [[nodiscard]] Data::Size      GetPoolSize() const noexcept final           { return m_pool_size; }
    [[nodiscard]] Data::Size      GetQuerySize() const noexcept final          { return m_query_size; }
    [[nodiscard]] IQuery::Count   GetSlotsCountPerQuery() const noexcept final { return m_slots_count_per_query; }
    [[nodiscard]] const IContext& GetContext() const noexcept final            { return m_context; }
    [[nodiscard]] ICommandQueue&  GetCommandQueue() noexcept final;

protected:
    QueryPoolBase(CommandQueueBase& command_queue, Type type,
                  IQuery::Count max_query_count, IQuery::Count slots_count_per_query,
                  Data::Size buffer_size, Data::Size query_size);

    friend class QueryBase;
    virtual void ReleaseQuery(const QueryBase& query);

    using CreateQueryArgs = std::tuple<IQuery::Index, IQuery::Range>;
    [[nodiscard]] CreateQueryArgs GetCreateQueryArguments();

    [[nodiscard]] CommandQueueBase& GetCommandQueueBase() noexcept { return m_command_queue; }

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    const Type          m_type;
    const Data::Size    m_pool_size;
    const Data::Size    m_query_size;
    const IQuery::Count m_slots_count_per_query;
    RangeSet            m_free_indices;
    RangeSet            m_free_data_ranges;
    CommandQueueBase&   m_command_queue;
    const IContext&     m_context;
};

class TimestampQueryPoolBase
    : public ITimestampQueryPool
{
public:
    // ITimestampQueryPool overrides
    Frequency                   GetGpuFrequency() const noexcept final           { return m_gpu_frequency; }
    const CalibratedTimestamps& GetCalibratedTimestamps() const noexcept final   { return m_calibrated_timestamps; }
    TimeDelta                   GetGpuTimeOffset() const noexcept final;

protected:
    void SetGpuFrequency(Frequency gpu_frequency);
    void SetCalibratedTimestamps(const CalibratedTimestamps& calibrated_timestamps);

private:
    Frequency            m_gpu_frequency = 0U;
    CalibratedTimestamps m_calibrated_timestamps{ 0U, 0U};
};

} // namespace Methane::Graphics
