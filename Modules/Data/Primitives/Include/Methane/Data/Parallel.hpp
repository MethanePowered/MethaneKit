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

template<typename T>
std::enable_if_t<std::is_unsigned<T>::value, T> DivCeil(T numerator, T denominator)
{
    return numerator > 0 ? (1 + ((numerator - 1) / denominator)) : 0;
}

template<typename T>
std::enable_if_t<std::is_signed<T>::value, T> DivCeil(T numerator, T denominator)
{
    std::div_t res = std::div(static_cast<int32_t>(numerator), static_cast<int32_t>(denominator));
    return res.rem ? (res.quot >= 0 ? (res.quot + 1) : (res.quot - 1))
                   : res.quot;
}

template<typename Iterator, typename Value>
struct IteratorFunction
{
    using type = std::function<void(Value&, Index)>;
};

#if 0 // TODO: fix or remove experimental code

template<typename Iterator, typename = std::void_t<typename std::iterator_traits<Iterator>::iterator_category>>
struct is_const_iterator
{
    static constexpr bool value = std::is_const<typename std::iterator_traits<Iterator>::value_type>::value;
};

template<typename Iterator>
struct IteratorFunction<Iterator, std::enable_if_t<is_const_iterator<Iterator>::value, const typename std::iterator_traits<Iterator>::value_type>>;

template<typename Iterator>
struct IteratorFunction<Iterator, std::enable_if_t<!is_const_iterator<Iterator>::value, typename std::iterator_traits<Iterator>::value_type>>;

#endif

template<typename Iterator, typename Value>
void ParallelFor(const Iterator& begin_it, const Iterator& end_it,
                 typename IteratorFunction<Iterator, Value>::type&& body_function)
{
    ITT_FUNCTION_TASK();

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
                        body_function(*(begin_it + index), static_cast<Index>(index));
                    }
                }
            )
        );
    }

    for(const std::future<void>& future : futures)
    {
        future.wait();
    };
}

template<typename IndexType, typename = std::enable_if_t<std::is_integral<IndexType>::value>>
void ParallelFor(IndexType start_index, IndexType count, std::function<void(IndexType)>&& body_function)
{
    ITT_FUNCTION_TASK();

    const IndexType hw_theads_count = static_cast<IndexType>(std::thread::hardware_concurrency());
    const IndexType chunk_size      = Data::DivCeil(count, hw_theads_count);

    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<size_t>(count));

    for (IndexType chunk_begin_index = start_index; chunk_begin_index < count; chunk_begin_index += chunk_size)
    {
        const IndexType chunk_end_index = std::min(chunk_begin_index + chunk_size, start_index + count);
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
    };
}

} // namespace Methane::Data