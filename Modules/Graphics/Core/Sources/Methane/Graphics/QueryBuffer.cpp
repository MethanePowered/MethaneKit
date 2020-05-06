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

FILE: Methane/Graphics/DirectX12/QueryBuffer.cpp
GPU data query buffer base implementation.

******************************************************************************/

#include "QueryBuffer.h"
#include "CommandQueueBase.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Data/RangeUtils.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

QueryBuffer::Query::Query(QueryBuffer& buffer, Data::Index index, Range data_range)
    : m_buffer(buffer)
    , m_index(index)
    , m_data_range(data_range)
{
    META_FUNCTION_TASK();
}

QueryBuffer::Query::~Query()
{
    META_FUNCTION_TASK();
    m_buffer.ReleaseQuery(*this);
}

QueryBuffer::QueryBuffer(CommandQueueBase& command_queue, Type type,
                         Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size)
    : m_type(type)
    , m_buffer_size(buffer_size)
    , m_query_size(query_size)
    , m_free_indices({ { 0u, max_query_count } })
    , m_free_data_ranges({ { 0u, buffer_size } })
    , m_command_queue(command_queue)
    , m_context(dynamic_cast<Context&>(command_queue.GetContext()))
{
    META_FUNCTION_TASK();
}

Ptr<QueryBuffer::Query> QueryBuffer::CreateQuery()
{
    META_FUNCTION_TASK();
    const Data::Range<Data::Index> index_range = Data::ReserveRange(m_free_indices, 1u);
    if (index_range.IsEmpty())
        throw std::out_of_range("Maximum queries count is reached.");

    const Query::Range data_range = Data::ReserveRange(m_free_data_ranges, m_query_size);
    if (index_range.IsEmpty())
        throw std::out_of_range("There is no space available for new query.");

    return std::make_shared<Query>(*this, index_range.GetStart(), data_range);
}

void QueryBuffer::ReleaseQuery(const Query& query)
{
    META_FUNCTION_TASK();
    m_free_indices.Add({ query.GetIndex(), query.GetIndex() + 1 });
    m_free_data_ranges.Add(query.GetDataRange());
}

Data::Size ITimestampQueryBuffer::GetTimestampResultBufferSize(const Context& context, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    const uint32_t frames_count = context.GetType() == Context::Type::Render
                                  ? dynamic_cast<const RenderContext&>(context).GetSettings().frame_buffers_count
                                  : 1u;
    return frames_count * max_timestamps_per_frame * sizeof(GpuTimestamp);
}

} // namespace Methane::Graphics