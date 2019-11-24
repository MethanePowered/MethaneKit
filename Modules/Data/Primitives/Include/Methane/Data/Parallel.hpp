/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Data/Parallel.hpp
Data parallel processing primitives.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>

#include <iterator>
#include <functional>
#include <future>

namespace Methane::Data
{

template<typename Iterator,
         typename Value = typename std::iterator_traits<Iterator>::value_type>
void ParallelFor(const Iterator& begin_it, const Iterator& end_it,
                 std::function<void(Value&, Index)>&& body_function)
{
    ITT_FUNCTION_TASK();

    std::vector<std::future<void>> futures;
    futures.reserve(std::distance(begin_it, end_it));

    for (Iterator it = begin_it; it != end_it; ++it)
    {
        const Index item_index = static_cast<Index>(std::distance(begin_it, it));
        futures.emplace_back(std::async(std::launch::async, body_function, std::ref(*it), item_index));
    }

    for(const std::future<void>& future : futures)
    {
        future.wait();
    };
}

template<typename ConstIterator,
         typename Value = typename std::iterator_traits<ConstIterator>::value_type>
void ParallelFor(const ConstIterator& begin_it, const ConstIterator& end_it,
                 std::function<void(const Value&, Index)>&& body_function)
{
    ITT_FUNCTION_TASK();

    std::vector<std::future<void>> futures;
    futures.reserve(std::distance(begin_it, end_it));

    for (ConstIterator it = begin_it; it != end_it; ++it)
    {
        const Index item_index = static_cast<Index>(std::distance(begin_it, it));
        futures.emplace_back(std::async(std::launch::async, body_function, std::cref(*it), item_index));
    }

    for (const std::future<void>& future : futures)
    {
        future.wait();
    };
}

template<typename IndexType>
void ParallelFor(IndexType start_index, IndexType count, std::function<void(IndexType)>&& body_function)
{
    ITT_FUNCTION_TASK();

    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<size_t>(count));

    const IndexType end_index = start_index + count;
    for (IndexType index = start_index; index < end_index; ++index)
    {
        futures.emplace_back(std::async(std::launch::async, body_function, index));
    }

    for (const std::future<void>& future : futures)
    {
        future.wait();
    };
}

} // namespace Methane::Data