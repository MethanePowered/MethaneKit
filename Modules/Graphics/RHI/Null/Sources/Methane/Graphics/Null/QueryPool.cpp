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

FILE: Methane/Graphics/Null/QueryPool.cpp
Null GPU query pool implementation.

******************************************************************************/

#include <Methane/Graphics/Null/QueryPool.h>
#include <Methane/Graphics/Null/CommandQueue.h>

namespace Methane::Graphics::Null
{

Query::Query(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range)
    : Base::Query(buffer, command_list, index, data_range)
{ }

TimestampQuery::TimestampQuery(Base::QueryPool& buffer, Base::CommandList& command_list, Index index, Range data_range)
    : Query(buffer, command_list, index, data_range)
{ }

TimestampQueryPool::TimestampQueryPool(CommandQueue& command_queue, uint32_t max_timestamps_per_frame)
    : Base::QueryPool(command_queue, Type::Timestamp, 1U << 15U, 1U, max_timestamps_per_frame * sizeof(Timestamp), sizeof(Timestamp))
{ }

} // namespace Methane::Graphics::Null
