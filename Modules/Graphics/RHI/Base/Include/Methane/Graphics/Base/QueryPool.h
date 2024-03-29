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

FILE: Methane/Graphics/Base/QueryPool.h
GPU data query pool base implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IQueryPool.h>
#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Data/Types.h>
#include <Methane/Data/TimeRange.hpp>
#include <Methane/Data/RangeSet.hpp>

namespace Methane::Graphics::Base
{

class CommandQueue;
class CommandList;
class QueryPool;

class Query // NOSONAR - custom destructor is required
    : public Rhi::IQuery
{
public:
    Query(QueryPool& buffer, CommandList& command_list, Index index, Range data_range);
    Query(const Query&) = delete;
    Query(Query&&) = delete;
    ~Query() override;

    // IQuery overrides
    void Begin() override;
    void End() override;
    void ResolveData() override;

    [[nodiscard]] Index              GetIndex() const noexcept final       { return m_index; }
    [[nodiscard]] const Range&       GetDataRange() const noexcept final   { return m_data_range; }
    [[nodiscard]] State              GetState() const noexcept final       { return m_state; }
    [[nodiscard]] Rhi::IQueryPool&   GetQueryPool() const noexcept final;
    [[nodiscard]] Rhi::ICommandList& GetCommandList() const noexcept final;

private:
    Ptr<QueryPool> m_query_pool_ptr;
    CommandList&   m_command_list;
    const Index    m_index;
    const Range    m_data_range;
    State          m_state = State::Resolved;
};

class QueryPool
    : public Rhi::IQueryPool
    , public std::enable_shared_from_this<QueryPool>
{
public:
    template<typename QueryT>
    [[nodiscard]] Ptr<QueryT> CreateQuery(CommandList& command_list)
    {
        const auto [query_index, query_range] = GetCreateQueryArguments();
        return std::make_shared<QueryT>(*this, command_list, query_index, query_range);
    }

    // IQueryPool overrides
    [[nodiscard]] Ptr<Rhi::IQueryPool> GetPtr() final                               { return std::dynamic_pointer_cast<IQueryPool>(shared_from_this()); }
    [[nodiscard]] Type                 GetType() const noexcept final               { return m_type; }
    [[nodiscard]] Data::Size           GetPoolSize() const noexcept final           { return m_pool_size; }
    [[nodiscard]] Data::Size           GetQuerySize() const noexcept final          { return m_query_size; }
    [[nodiscard]] Rhi::IQuery::Count   GetSlotsCountPerQuery() const noexcept final { return m_slots_count_per_query; }
    [[nodiscard]] const Rhi::IContext& GetContext() const noexcept final            { return m_context; }
    [[nodiscard]] Rhi::ICommandQueue&  GetCommandQueue() noexcept final;

protected:
    QueryPool(CommandQueue& command_queue, Type type,
              Rhi::IQuery::Count max_query_count, Rhi::IQuery::Count slots_count_per_query,
              Data::Size buffer_size, Data::Size query_size);

    friend class Query;
    virtual void ReleaseQuery(const Query& query);

    using CreateQueryArgs = std::tuple<Rhi::IQuery::Index, Rhi::IQuery::Range>;
    [[nodiscard]] CreateQueryArgs GetCreateQueryArguments();

    [[nodiscard]] CommandQueue& GetBaseCommandQueue() noexcept { return m_command_queue; }

private:
    using RangeSet = Data::RangeSet<Data::Index>;

    const Type               m_type;
    const Data::Size         m_pool_size;
    const Data::Size         m_query_size;
    const Rhi::IQuery::Count m_slots_count_per_query;
    RangeSet                 m_free_indices;
    RangeSet                 m_free_data_ranges;
    CommandQueue&            m_command_queue;
    const Rhi::IContext&     m_context;
};

class TimestampQueryPool
    : public Rhi::ITimestampQueryPool
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

} // namespace Methane::Graphics::Base
