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

#include <mutex>
#include <thread>

namespace Methane::Data
{

template<typename Iterator, class UnaryFunction>
void ParallelFor(const Iterator& begin_it, const Iterator& end_it, UnaryFunction&& f)
{
    ITT_FUNCTION_TASK();

    const uint32_t items_count = static_cast<uint32_t>(std::distance(begin_it, end_it));
    const uint32_t thread_count = std::thread::hardware_concurrency();
    const uint32_t group_size = std::max(1u, static_cast<uint32_t>(std::ceil(static_cast<float>(items_count) / thread_count)));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (Iterator it = begin_it; static_cast<uint32_t>(std::distance(begin_it, it)) < items_count; it += group_size)
    {
        threads.push_back(
            std::thread([=, &f]()
            {
                std::for_each(it, std::min(it + group_size, end_it), f);
            }));
    }

    std::for_each(threads.begin(), threads.end(), [](std::thread& thread)
    {
        thread.join();
    });
}

} // namespace Methane::Data