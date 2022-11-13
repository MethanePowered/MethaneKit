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

#include <Methane/Graphics/Vulkan/QueryPool.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>
#include <Methane/Graphics/Vulkan/CommandList.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>

#include <Methane/Graphics/Base/QueryPool.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <chrono>
#include <magic_enum.hpp>

static const vk::TimeDomainEXT g_vk_cpu_time_domain =
#if defined(_WIN32)
    vk::TimeDomainEXT::eQueryPerformanceCounter;
#elif defined(__linux__) && defined CLOCK_MONOTONIC_RAW
    vk::TimeDomainEXT::eClockMonotonicRaw;
#else
    static_cast<vk::TimeDomainEXT>(-1);
#endif

namespace Methane::Graphics::Rhi
{

Ptr<ITimestampQueryPool> Rhi::ITimestampQueryPool::Create(ICommandQueue& command_queue, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::TimestampQueryPool>(dynamic_cast<Vulkan::CommandQueue&>(command_queue), max_timestamps_per_frame);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

static vk::QueryType GetQueryTypeVk(Rhi::IQueryPool::Type query_pool_type)
{
    META_FUNCTION_TASK();
    switch(query_pool_type) // NOSONAR
    {
    case Rhi::IQueryPool::Type::Timestamp: return vk::QueryType::eTimestamp;
    // vk::QueryType::eOcclusion
    // vk::QueryType::ePipelineStatistics
    default: META_UNEXPECTED_ARG_RETURN(query_pool_type, vk::QueryType::eTimestamp);
    }
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
    , m_vk_device(GetVulkanQueryPool().GetVulkanContext().GetVulkanDevice().GetNativeDevice())
    , m_vk_command_buffer(dynamic_cast<ICommandListVk&>(command_list).GetNativeCommandBuffer(ICommandListVk::CommandBufferType::Primary))
    , m_query_results(buffer.GetSlotsCountPerQuery(), 0U)
    , m_query_results_byte_size(m_query_results.size() * sizeof(QueryResults::value_type))
{
    META_FUNCTION_TASK();
}

void Query::Begin()
{
    META_FUNCTION_TASK();
    Base::Query::Begin();
    const vk::QueryPool& vk_query_pool = GetVulkanQueryPool().GetNativeQueryPool();
    m_vk_command_buffer.resetQueryPool(vk_query_pool, GetIndex(), GetQueryPool().GetSlotsCountPerQuery());
    m_vk_command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, vk_query_pool, GetIndex());
}

void Query::End()
{
    META_FUNCTION_TASK();
    Base::Query::End();
    m_vk_command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, GetVulkanQueryPool().GetNativeQueryPool(), GetIndex());
}

Rhi::SubResource Query::GetData() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL_DESCR(GetState(), Rhi::IQuery::State::Resolved, "query data can be retrieved only from resolved query");
    META_CHECK_ARG_EQUAL_DESCR(GetCommandList().GetState(), Base::CommandList::State::Pending, "query data can be retrieved only when command list is in Pending/Completed state");

    vk::Result vk_query_result = m_vk_device.getQueryPoolResults(
        GetVulkanQueryPool().GetNativeQueryPool(), GetIndex(), GetQueryPool().GetSlotsCountPerQuery(),
        m_query_results_byte_size, m_query_results.data(), sizeof(QueryResults::value_type),
        vk::QueryResultFlagBits::e64);
    META_CHECK_ARG_EQUAL_DESCR(vk_query_result, vk::Result::eSuccess, "failed to get query pool results");

    return Rhi::SubResource(reinterpret_cast<Data::ConstRawPtr>(m_query_results.data()), // NOSONAR
                            static_cast<Data::Size>(m_query_results_byte_size));
}

QueryPool& Query::GetVulkanQueryPool() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<QueryPool&>(GetQueryPool());
}

QueryPool::QueryPool(CommandQueue& command_queue, Type type,
                         Data::Size max_query_count, Base::Query::Count slots_count_per_query,
                         Data::Size buffer_size, Data::Size query_size)
    : Base::QueryPool(command_queue, type, max_query_count, slots_count_per_query, buffer_size, query_size)
    , m_context_vk(dynamic_cast<const IContext&>(GetContext()))
    , m_vk_query_pool(command_queue.GetVulkanDevice().GetNativeDevice().createQueryPool(vk::QueryPoolCreateInfo({}, GetQueryTypeVk(type), max_query_count)))
{
    META_FUNCTION_TASK();
}

CommandQueue& QueryPool::GetVulkanCommandQueue() noexcept
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

    // Check GPU device frequency
    using namespace std::chrono_literals;
    const vk::Device& vk_device = command_queue.GetVulkanDevice().GetNativeDevice();
    const vk::PhysicalDevice& vk_physical_device = command_queue.GetVulkanDevice().GetNativePhysicalDevice();
    const float gpu_timestamp_period = vk_physical_device.getProperties().limits.timestampPeriod;
    SetGpuFrequency(static_cast<Frequency>(gpu_timestamp_period * std::chrono::nanoseconds(1s).count()));

    // Check if Vulkan supports CPU time domains calibration
    const auto calibrateable_time_domains = vk_physical_device.getCalibrateableTimeDomainsEXT();
    bool is_cpu_time_domain_calibrateable = std::find(calibrateable_time_domains.begin(), calibrateable_time_domains.end(), g_vk_cpu_time_domain) != calibrateable_time_domains.end();
    META_CHECK_ARG_TRUE_DESCR(is_cpu_time_domain_calibrateable, "Vulkan does not support calibration of the CPU time domain {}", magic_enum::enum_name(g_vk_cpu_time_domain));

    // Calculate the desired CPU-GPU timestamps deviation
    const std::array<vk::CalibratedTimestampInfoEXT, 2> timestamp_infos = {{ { vk::TimeDomainEXT::eDevice }, { g_vk_cpu_time_domain }, }};
    std::array<uint64_t, 2>  timestamps{{}};
    std::array<uint64_t, 32> probe_deviations{{}};
    for(uint64_t& deviation : probe_deviations)
    {
        const vk::Result vk_calibrate_result = vk_device.getCalibratedTimestampsEXT(static_cast<uint32_t>(timestamp_infos.size()), timestamp_infos.data(), timestamps.data(), &deviation);
        META_CHECK_ARG_EQUAL(vk_calibrate_result, vk::Result::eSuccess);
    }
    uint64_t min_deviation = std::numeric_limits<uint64_t>::max();
    for(uint64_t deviation : probe_deviations)
    {
        min_deviation = std::min(min_deviation, deviation);
    }
    m_deviation = min_deviation * 3 / 2;

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
    const vk::Device& vk_device = GetVulkanCommandQueue().GetVulkanDevice().GetNativeDevice();
    const std::array<vk::CalibratedTimestampInfoEXT, 2> timestamp_infos = {{ { vk::TimeDomainEXT::eDevice }, { g_vk_cpu_time_domain }, }};
    std::array<uint64_t, 2> timestamps{{}};
    uint64_t deviation = 0U;

    do
    {
        const vk::Result vk_calibrate_result = vk_device.getCalibratedTimestampsEXT(static_cast<uint32_t>(timestamp_infos.size()), timestamp_infos.data(), timestamps.data(), &deviation);
        META_CHECK_ARG_EQUAL(vk_calibrate_result, vk::Result::eSuccess);
    }
    while(deviation > m_deviation);

    CalibratedTimestamps calibrated_timestamps{};
    calibrated_timestamps.gpu_ts = timestamps[0];
    calibrated_timestamps.cpu_ts = timestamps[1] * Data::GetQpcToNSecMultiplier();
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
    GetVulkanCommandBuffer().resetQueryPool(GetVulkanQueryPool().GetNativeQueryPool(), GetIndex(), 1U);
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
    const TimestampQueryPool& timestamp_query_pool_vk = GetVulkanTimestampQueryPool();
    const Timestamp gpu_timestamp = TimestampQuery::GetGpuTimestamp();
    return Data::ConvertTicksToNanoseconds(gpu_timestamp - timestamp_query_pool_vk.GetGpuTimeOffset(), timestamp_query_pool_vk.GetGpuFrequency());
}

TimestampQueryPool& TimestampQuery::GetVulkanTimestampQueryPool() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<TimestampQueryPool&>(GetQueryPool());
}

} // namespace Methane::Graphics::Vulkan
