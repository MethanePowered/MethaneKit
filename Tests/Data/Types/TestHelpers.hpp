/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Tests/Data/Types/TestHelpers.hpp
Shared unit test helpers for data types

******************************************************************************/

#pragma once

#include <Methane/Data/Vector.hpp>

#define VECTOR_TYPES_MATRIX \
    ((typename T, size_t size), T, size),        \
    (int32_t,  2), (int32_t,  3), (int32_t,  4), \
    (uint32_t, 2), (uint32_t, 3), (uint32_t, 4), \
    (float,    2), (float,    3), (float,    4), \
    (double,   2), (double,   3), (double,   4)

namespace Methane::Data
{

template<typename T, size_t size>
std::array<T, size> CreateEqualComponents(T value = T(1))
{
    std::array <T, size> values{};
    values.fill(value);
    return values;
}

template<typename T, size_t size>
std::array<T, size> CreateComponents(T first_value = T(1), T step_value = T(1))
{
    std::array<T, size> values{ first_value };
    for (size_t i = 1; i < size; ++i)
    {
        values[i] = first_value + step_value * T(i);
    }
    return values;
}

template<typename T, typename V, size_t size, typename DoFunc /* [](T left_component, V right_component) -> T */>
std::array<T, size> DoPerComponent(const std::array<T, size>& left, const std::array<V, size>& right, const DoFunc& do_func)
{
    std::array<T, size> result{};
    for (size_t i = 0; i < size; ++i)
    {
        result[i] = do_func(left[i], right[i]);
    }
    return result;
}

template<typename T, size_t size>
HlslVector<T, size> CreateHlslVector(const std::array <T, size>& components)
{
    if constexpr (size == 2)
        return HlslVector<T, 2>(components[0], components[1]);
    if constexpr (size == 3)
        return HlslVector<T, 3>(components[0], components[1], components[2]);
    if constexpr (size == 4)
        return HlslVector<T, 4>(components[0], components[1], components[2], components[3]);
}

} // namespace Methane::Data
