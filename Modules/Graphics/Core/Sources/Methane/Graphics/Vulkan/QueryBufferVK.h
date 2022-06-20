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

FILE: Methane/Graphics/Vulkan/QueryBufferVK.hpp
Vulkan GPU query results buffer.

******************************************************************************/

#pragma once

#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Memory.hpp>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct IContextVK;
class  CommandQueueVK;

class QueryBufferVK : public QueryBuffer
{
public:
    class QueryVK : public Query
    {
    public:
        QueryVK(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range);

        // Query overrides
        void Begin() override;
        void End() override;
        void ResolveData() override;
        Resource::SubResource GetData() override;

    protected:
        [[nodiscard]] QueryBufferVK& GetQueryBufferVK() noexcept  { return static_cast<QueryBufferVK&>(GetQueryBuffer()); }

    private:
    };

    QueryBufferVK(CommandQueueVK& command_queue, Type type,
                  Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size);

    CommandQueueVK&   GetCommandQueueVK() noexcept;
    const IContextVK& GetContextDX() const noexcept { return m_context_vk; }

private:
    const IContextVK& m_context_vk;
    vk::QueryPool     m_vk_query_pool;
};

using GpuTimeCalibration = std::pair<Timestamp, TimeDelta>;

class TimestampQueryBufferVK final
    : public QueryBufferVK
    , public TimestampQueryBuffer
{
public:
    class TimestampQueryVK final
        : public QueryVK
        , public TimestampQuery
    {
    public:
        TimestampQueryVK(QueryBuffer& buffer, CommandListBase& command_list, Index index, Range data_range);

        // TimestampQuery overrides
        void InsertTimestamp() override;
        void ResolveTimestamp() override;
        Timestamp GetGpuTimestamp() override;
        Timestamp GetCpuNanoseconds() override;

    private:
        [[nodiscard]] TimestampQueryBufferVK& GetTimestampQueryBufferVK() noexcept { return static_cast<TimestampQueryBufferVK&>(GetQueryBuffer()); }
    };

    TimestampQueryBufferVK(CommandQueueVK& command_queue, uint32_t max_timestamps_per_frame);

    // ITimestampQueryBuffer interface
    Ptr<TimestampQuery> CreateTimestampQuery(CommandListBase& command_list) override;

    Frequency GetGpuFrequency() const noexcept            { return m_gpu_frequency; }
    TimeDelta GetGpuTimeOffset() const noexcept           { return m_gpu_time_calibration.second; }
    TimeDelta GetGpuCalibrationTimestamp() const noexcept { return m_gpu_time_calibration.first; }

private:
    const Frequency          m_gpu_frequency;
    const GpuTimeCalibration m_gpu_time_calibration;
};

} // namespace Methane::Graphics
