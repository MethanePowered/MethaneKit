/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/QueryPoolVK.hpp
Vulkan GPU query pool implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/QueryPool.h>
#include <Methane/Memory.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct IContextVK;
struct ICommandListVK;
class  CommandQueueVK;
class  QueryPoolVK;
class  TimestampQueryPoolVK;

class QueryVK : public Base::Query
{
public:
    QueryVK(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // Query overrides
    void Begin() final;
    void End() final;
    [[nodiscard]] IResource::SubResource GetData() const final;

protected:
    [[nodiscard]] QueryPoolVK& GetQueryPoolVK() const noexcept;
    [[nodiscard]] const vk::CommandBuffer& GetCommandBufferVK() const noexcept { return m_vk_command_buffer; }

private:
    using QueryResults = std::vector<uint64_t>;

    const vk::Device        m_vk_device;
    const vk::CommandBuffer m_vk_command_buffer;
    mutable QueryResults    m_query_results;
    size_t                  m_query_results_byte_size;
};

class QueryPoolVK : public Base::QueryPool
{
public:
    QueryPoolVK(CommandQueueVK& command_queue, Type type,
                Data::Size max_query_count, IQuery::Count slots_count_per_query,
                Data::Size buffer_size, Data::Size query_size);

    CommandQueueVK&      GetCommandQueueVK() noexcept;
    const IContextVK&    GetContextVK() const noexcept       { return m_context_vk; }
    const vk::QueryPool& GetNativeQueryPool() const noexcept { return m_vk_query_pool; }

private:
    const IContextVK& m_context_vk;
    vk::QueryPool     m_vk_query_pool;
};

class TimestampQueryVK final
    : protected QueryVK
    , public ITimestampQuery
{
public:
    TimestampQueryVK(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // TimestampQuery overrides
    void InsertTimestamp() override;
    void ResolveTimestamp() override;
    Timestamp GetGpuTimestamp() const override;
    Timestamp GetCpuNanoseconds() const override;

private:
    [[nodiscard]] TimestampQueryPoolVK& GetTimestampQueryPoolVK() const noexcept;
};

class TimestampQueryPoolVK final
    : public QueryPoolVK
    , public Base::TimestampQueryPool
{
public:
    TimestampQueryPoolVK(CommandQueueVK& command_queue, uint32_t max_timestamps_per_frame);

    // ITimestampQueryPool interface
    Ptr<ITimestampQuery> CreateTimestampQuery(ICommandList& command_list) override;
    CalibratedTimestamps Calibrate() override;

private:
    uint64_t                m_deviation = 0U;
};

} // namespace Methane::Graphics
