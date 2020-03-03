/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Data/Posix/Parallel.hpp
Data parallel processing primitives
- on Windows platform: implemented with Parallel Primitives Library (PPL)
- on Posix platforms: implemented with STL Async and Future.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>

#ifdef _WIN32
#define METHANE_USE_PPL
#endif

#ifdef METHANE_USE_PPL

#include <ppl.h>

#else

#include <Methane/Data/Math.hpp>
#include <future>

#endif

#include <type_traits>
#include <iterator>
#include <functional>

namespace Methane::Data
{

template<typename Iterator, typename Value>
struct IteratorFunction
{
    using Type = std::function<void(Value&)>;
};

template<typename Iterator, typename Value>
void ParallelForEach(const Iterator& begin_it, const Iterator& end_it,
                     typename IteratorFunction<Iterator, Value>::Type&& body_function)
{
    ITT_FUNCTION_TASK();

#ifdef METHANE_USE_PPL

    concurrency::parallel_for_each(begin_it, end_it, body_function);

#else

    const size_t items_count     = std::distance(begin_it, end_it);
    const size_t hw_theads_count = std::thread::hardware_concurrency();
    const size_t chunk_size      = Data::DivCeil(items_count, hw_theads_count);

    std::vector<std::future<void>> futures;
    futures.reserve(items_count);

    for (size_t chunk_begin_index = 0; chunk_begin_index < items_count; chunk_begin_index += chunk_size)
    {
        const size_t chunk_end_index = std::min(chunk_begin_index + chunk_size, items_count);
        futures.emplace_back(
            std::async(std::launch::async,
                [&begin_it, chunk_begin_index, chunk_end_index, body_function]()
                {
                    for(size_t index = chunk_begin_index; index < chunk_end_index; ++index)
                    {
                        body_function(*(begin_it + index));
                    }
                }
            )
        );
    }

    for(const std::future<void>& future : futures)
    {
        future.wait();
    }

#endif
}

template<typename IndexType, typename = std::enable_if_t<std::is_integral<IndexType>::value>>
void ParallelFor(IndexType begin_index, IndexType end_index, std::function<void(IndexType)>&& body_function)
{
    ITT_FUNCTION_TASK();

    if (end_index < begin_index)
    {
        throw std::invalid_argument("ParallelFor requires end_index to be greater or equal to begin_index");
    }

#ifdef METHANE_USE_PPL

    concurrency::parallel_for(begin_index, end_index, body_function);

#else

    const IndexType count            = end_index - begin_index;
    const IndexType hw_threads_count = static_cast<IndexType>(std::thread::hardware_concurrency());
    const IndexType chunk_size       = Data::DivCeil(count, hw_threads_count);

    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<size_t>(count));

    for (IndexType chunk_begin_index = begin_index; chunk_begin_index < count; chunk_begin_index += chunk_size)
    {
        const IndexType chunk_end_index = std::min(chunk_begin_index + chunk_size, end_index);
        futures.emplace_back(
            std::async(std::launch::async,
                [chunk_begin_index, chunk_end_index, body_function]()
                {
                    for (IndexType index = chunk_begin_index; index < chunk_end_index; ++index)
                    {
                        body_function(index);
                    }
                }
            )
        );
    }

    for (const std::future<void>& future : futures)
    {
        future.wait();
    }

#endif
}

} // namespace Methane::Data