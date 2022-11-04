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

FILE: Methane/Graphics/QueryPool.h
GPU data query pool private interface.

******************************************************************************/

#pragma once

#include "ResourceView.h"

#include <Methane/Data/Types.h>

namespace Methane::Graphics
{

struct ICommandQueue;
struct ICommandList;
struct IContext;
struct IQueryPool;
struct ITimestampQuery;

struct IQuery
{
    using Index = Data::Index;
    using Count = Data::Size;
    using Range = Data::Range<Data::Index>;

    enum class State
    {
        Resolved = 0,
        Begun,
        Ended,
    };

    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual void ResolveData() = 0;

    [[nodiscard]] virtual Index         GetIndex() const noexcept = 0;
    [[nodiscard]] virtual const Range&  GetDataRange() const noexcept = 0;
    [[nodiscard]] virtual State         GetState() const noexcept = 0;
    [[nodiscard]] virtual SubResource   GetData() const = 0;
    [[nodiscard]] virtual IQueryPool&   GetQueryPool() const noexcept = 0;
    [[nodiscard]] virtual ICommandList& GetCommandList() const noexcept = 0;

    virtual ~IQuery() = default;
};

struct IQueryPool
{
    enum class Type
    {
        Timestamp,
    };

    [[nodiscard]] virtual Ptr<ITimestampQuery> CreateTimestampQuery(ICommandList& command_list) = 0;
    [[nodiscard]] virtual Ptr<IQueryPool>      GetPtr() = 0;
    [[nodiscard]] virtual Type                 GetType() const noexcept = 0;
    [[nodiscard]] virtual Data::Size           GetPoolSize() const noexcept = 0;
    [[nodiscard]] virtual Data::Size           GetQuerySize() const noexcept = 0;
    [[nodiscard]] virtual IQuery::Count        GetSlotsCountPerQuery() const = 0;
    [[nodiscard]] virtual ICommandQueue&       GetCommandQueue() noexcept = 0;
    [[nodiscard]] virtual const IContext&      GetContext() const noexcept = 0;

    virtual ~IQueryPool() = default;
};

struct ITimestampQuery
{
    virtual void InsertTimestamp() = 0;
    virtual void ResolveTimestamp() = 0;

    [[nodiscard]] virtual Data::Timestamp GetGpuTimestamp() const = 0;
    [[nodiscard]] virtual Data::Timestamp GetCpuNanoseconds() const = 0;

    virtual ~ITimestampQuery() = default;
};

struct ITimestampQueryPool
{
    struct CalibratedTimestamps
    {
        Data::Timestamp gpu_ts;
        Data::Timestamp cpu_ts;
    };

    [[nodiscard]] static Ptr<ITimestampQueryPool> Create(ICommandQueue& command_queue, uint32_t max_timestamps_per_frame);

    [[nodiscard]] virtual Ptr<ITimestampQuery>        CreateTimestampQuery(ICommandList& command_list) = 0;
                  virtual CalibratedTimestamps        Calibrate() = 0;
    [[nodiscard]] virtual Data::Frequency             GetGpuFrequency() const noexcept = 0;
    [[nodiscard]] virtual const CalibratedTimestamps& GetCalibratedTimestamps() const noexcept = 0;
    [[nodiscard]] virtual Data::TimeDelta             GetGpuTimeOffset() const noexcept = 0;

    virtual ~ITimestampQueryPool() = default;
};

} // namespace Methane::Graphics
