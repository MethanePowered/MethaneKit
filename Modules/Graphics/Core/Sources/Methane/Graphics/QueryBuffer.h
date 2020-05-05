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

    QueryBuffer(CommandQueueBase& command_queue, Type type);
    virtual ~QueryBuffer() = default;

    Type              GetType() const noexcept       { return m_type; }
    CommandQueueBase& GetCommandQueueBase() noexcept { return m_command_queue; }
    Context&          GetContext() noexcept          { return m_context; }

private:
    const Type        m_type;
    CommandQueueBase& m_command_queue;
    Context&          m_context;
};

using GpuTimestamp = uint64_t;
using GpuFrequency = uint64_t;

struct ITimestampQueryBuffer
{
    virtual GpuFrequency GetGpuFrequency() const noexcept = 0;
    virtual ~ITimestampQueryBuffer() = default;
};

class TimestampQueryBufferDummy final
    : public QueryBuffer
    , public ITimestampQueryBuffer
{
public:
    explicit TimestampQueryBufferDummy(CommandQueueBase& command_queue, uint32_t max_timestamps_per_frame);

    GpuFrequency GetGpuFrequency() const noexcept override { return 0u; }
};

} // namespace Methane::Graphics
