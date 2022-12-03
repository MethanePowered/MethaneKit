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

FILE: Methane/Graphics/Vulkan/QueryPool.hpp
Vulkan GPU query pool implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/QueryPool.h>
#include <Methane/Memory.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

struct IContext;
struct ICommandList;
class  CommandQueue;
class  QueryPool;
class  TimestampQueryPool;

class Query
    : public Base::Query
{
public:
    Query(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // Query overrides
    void Begin() final;
    void End() final;
    [[nodiscard]] Rhi::SubResource GetData() const final;

protected:
    [[nodiscard]] QueryPool& GetVulkanQueryPool() const noexcept;
    [[nodiscard]] const vk::CommandBuffer& GetVulkanCommandBuffer() const noexcept { return m_vk_command_buffer; }

private:
    using QueryResults = std::vector<uint64_t>;

    const vk::Device        m_vk_device;
    const vk::CommandBuffer m_vk_command_buffer;
    mutable QueryResults    m_query_results;
    size_t                  m_query_results_byte_size;
};

class QueryPool : public Base::QueryPool
{
public:
    QueryPool(CommandQueue& command_queue, Type type,
                Data::Size max_query_count, Rhi::IQuery::Count slots_count_per_query,
                Data::Size buffer_size, Data::Size query_size);

    CommandQueue&        GetVulkanCommandQueue() noexcept;
    const IContext&      GetVulkanContext() const noexcept   { return m_context_vk; }
    const vk::QueryPool& GetNativeQueryPool() const noexcept { return m_vk_query_pool; }

private:
    const IContext& m_context_vk;
    vk::QueryPool   m_vk_query_pool;
};

class TimestampQuery final
    : protected Query
    , public Rhi::ITimestampQuery
{
public:
    TimestampQuery(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // TimestampQuery overrides
    void InsertTimestamp() override;
    void ResolveTimestamp() override;
    Timestamp GetGpuTimestamp() const override;
    Timestamp GetCpuNanoseconds() const override;

private:
    [[nodiscard]] TimestampQueryPool& GetVulkanTimestampQueryPool() const noexcept;
};

class TimestampQueryPool final
    : public QueryPool
    , public Base::TimestampQueryPool
{
public:
    TimestampQueryPool(CommandQueue& command_queue, uint32_t max_timestamps_per_frame);

    // ITimestampQueryPool interface
    Ptr<Rhi::ITimestampQuery> CreateTimestampQuery(Rhi::ICommandList& command_list) override;
    CalibratedTimestamps Calibrate() override;

private:
    uint64_t                m_deviation = 0U;
};

} // namespace Methane::Graphics::Vulkan
