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

FILE: Methane/Graphics/QueryBuffer.cpp
GPU data query buffer base implementation.

******************************************************************************/

#include "QueryBuffer.h"
#include "CommandQueueBase.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContext.h>
#include <Methane/Data/RangeUtils.hpp>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

QueryBuffer::Query::Query(QueryBuffer& buffer, CommandListBase& command_list, Data::Index index, Range data_range)
    : m_buffer_ptr(buffer.GetPtr())
    , m_command_list(command_list)
    , m_index(index)
    , m_data_range(data_range)
{
    META_FUNCTION_TASK();
}

QueryBuffer::Query::~Query()
{
    META_FUNCTION_TASK();
    m_buffer_ptr->ReleaseQuery(*this);
}

void QueryBuffer::Query::Begin()
{
    META_FUNCTION_TASK();
    if (GetQueryBuffer().GetType() == QueryBuffer::Type::Timestamp)
        throw std::logic_error("Timestamp query can not be begun, it can be ended only.");

    if (m_state == State::Begun)
        throw std::logic_error("Can not begin unresolved or not ended query.");

    m_state = State::Begun;
}

void QueryBuffer::Query::End()
{
    META_FUNCTION_TASK();
    const QueryBuffer::Type query_type = GetQueryBuffer().GetType();
    if (query_type != QueryBuffer::Type::Timestamp && m_state != State::Begun)
        throw std::logic_error("Can not end " + GetTypeName(query_type) + " query that was not begun.");

    m_state = State::Ended;
}

void QueryBuffer::Query::ResolveData()
{
    META_FUNCTION_TASK();
    if (m_state != State::Ended)
        throw std::logic_error("Can not resolve data of not ended query.");

    m_state = State::Resolved;
}

QueryBuffer::QueryBuffer(CommandQueueBase& command_queue, Type type,
                         Data::Size max_query_count, Data::Size buffer_size, Data::Size query_size)
    : m_type(type)
    , m_buffer_size(buffer_size)
    , m_query_size(query_size)
    , m_free_indices({ { 0U, max_query_count } })
    , m_free_data_ranges({ { 0U, buffer_size } })
    , m_command_queue(command_queue)
    , m_context(dynamic_cast<Context&>(command_queue.GetContext()))
{
    META_FUNCTION_TASK();
}

void QueryBuffer::ReleaseQuery(const Query& query)
{
    META_FUNCTION_TASK();
    m_free_indices.Add({ query.GetIndex(), query.GetIndex() + 1 });
    m_free_data_ranges.Add(query.GetDataRange());
}

QueryBuffer::CreateQueryArgs QueryBuffer::GetCreateQueryArguments()
{
    META_FUNCTION_TASK();
    const Data::Range<Data::Index> index_range = Data::ReserveRange(m_free_indices, 1U);
    if (index_range.IsEmpty())
        throw std::out_of_range("Maximum queries count is reached.");

    const Query::Range data_range = Data::ReserveRange(m_free_data_ranges, m_query_size);
    if (index_range.IsEmpty())
        throw std::out_of_range("There is no space available for new query.");

    return { index_range.GetStart(), data_range };
}

std::string QueryBuffer::GetTypeName(Type type) noexcept
{
    META_FUNCTION_TASK();
    switch(type)
    {
    case Type::Timestamp: return "Timestamp";
    default: assert(0);
    }
    return "Unknown";
}

} // namespace Methane::Graphics