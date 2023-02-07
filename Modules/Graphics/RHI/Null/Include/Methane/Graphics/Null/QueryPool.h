/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/QueryPool.h
Null GPU query pool implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/QueryPool.h>

namespace Methane::Graphics::Null
{

class  CommandQueue;
class  QueryPool;
class  TimestampQueryPool;

class Query : public Base::Query
{
public:
    Query(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // Query overrides
    void Begin() override                     { /* Null implementation */ }
    void End() override                       { /* Null implementation */ }
    void ResolveData() override               { /* Null implementation */ }
    Rhi::SubResource GetData() const override { return {}; }
};

class TimestampQuery final
    : protected Query
    , public Rhi::ITimestampQuery
{
public:
    TimestampQuery(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range);

    // TimestampQuery overrides
    void InsertTimestamp() override                 { /* Null implementation */ }
    void ResolveTimestamp() override                { /* Null implementation */ }
    Timestamp GetGpuTimestamp() const override      { return 0U; }
    Timestamp GetCpuNanoseconds() const override    { return 0U; }
};

class TimestampQueryPool final
    : public Base::QueryPool
    , public Base::TimestampQueryPool
{
public:
    TimestampQueryPool(CommandQueue& command_queue, uint32_t max_timestamps_per_frame);

    // ITimestampQueryPool interface
    Ptr<Rhi::ITimestampQuery> CreateTimestampQuery(Rhi::ICommandList&) override { return nullptr; }
    CalibratedTimestamps Calibrate() override { return {}; }
};

} // namespace Methane::Graphics::Null
