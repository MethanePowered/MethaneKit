/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Data/Math.hpp
Math primitive functions.

******************************************************************************/

#pragma once

#include <type_traits>
#include <thread>

namespace Methane::Data
{

template<typename T>
std::enable_if_t<std::is_arithmetic<T>::value, T> AbsSubtract(T a, T b)
{
    return a >= b ? a - b : b - a;
}

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

template<typename T>
std::enable_if_t<std::is_integral<T>::value, T> GetParallelChunkSize(T items_count, T thread_granularity = 1)
{
    const size_t hw_theads_count = std::thread::hardware_concurrency();
    return Data::DivCeil(items_count, static_cast<T>(hw_theads_count) * thread_granularity);
}

template<typename T, typename G = T>
std::enable_if_t<std::is_integral<T>::value, int> GetParallelChunkSizeAsInt(T items_count, G thread_granularity = 1)
{
    return static_cast<int>(GetParallelChunkSize(items_count, static_cast<T>(thread_granularity)));
}

} // namespace Methane::Data