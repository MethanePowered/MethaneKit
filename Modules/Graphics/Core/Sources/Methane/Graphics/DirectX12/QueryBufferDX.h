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

FILE: Methane/Graphics/DirectX12/QueryBufferDX.hpp
DirectX 12 GPU query results buffer.

******************************************************************************/

#pragma once

#include <Methane/Graphics/QueryBuffer.h>
#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct Buffer;
struct IContextDX;
class  CommandQueueDX;
class  ResourceDX;

class QueryBufferDX : public QueryBuffer
{
public:
    QueryBufferDX(CommandQueueDX& command_queue, Type type, Data::Size buffer_size);

    CommandQueueDX&   GetCommandQueueDX() noexcept;
    const IContextDX& GetContextDX() noexcept              { return m_context_dx; }
    const ResourceDX& GetResultResourceDX() const noexcept { return m_result_resource_dx; }

protected:
    Buffer&           GetResultBuffer() noexcept           { return *m_sp_result_buffer; }

private:
    Ptr<Buffer>       m_sp_result_buffer;
    const ResourceDX& m_result_resource_dx;
    const IContextDX& m_context_dx;
};

class TimestampQueryBufferDX final
    : public QueryBufferDX
    , public ITimestampQueryBuffer
{
public:
    explicit TimestampQueryBufferDX(CommandQueueDX& command_queue, uint32_t max_timestamps_per_frame);

    GpuFrequency GetGpuFrequency() const noexcept override { return m_gpu_frequency; }

private:
    const uint32_t     m_max_timestamps_per_frame;
    const GpuTimestamp m_gpu_frequency;
};

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
#define TIMESTAMP_QUERY_BUFFER TimestampQueryBufferDX
#else
#define TIMESTAMP_QUERY_BUFFER TimestampQueryBufferDummy
#endif

} // namespace Methane::Graphics
