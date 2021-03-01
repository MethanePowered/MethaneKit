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

FILE: Methane/Data/Parallel.hpp
Parallel primitive functions.

******************************************************************************/

#pragma once

#include <Methane/Data/Math.hpp>

#include <thread>

namespace Methane::Data
{

template<typename T, typename G = T, typename R = T>
std::enable_if_t<std::is_integral_v<T>, R> GetParallelChunkSize(T items_count, G thread_granularity = G(1))
{
    static const size_t s_hw_theads_count = std::thread::hardware_concurrency();
    const T chunk_size = Data::DivCeil(items_count, RoundCast<T>(static_cast<G>(s_hw_theads_count) * thread_granularity));
    if constexpr (std::is_same_v<R, T>)
        return chunk_size;
    else
        return static_cast<R>(chunk_size);
}

} // namespace Methane::Data