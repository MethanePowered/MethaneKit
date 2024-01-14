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

FILE: Methane/Data/TimeRange.hpp
Methane time range type definition

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>
#include <Methane/Data/Range.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Methane::Data
{

using TimeRange = Range<Timestamp>;

constexpr Timestamp g_one_sec_in_nanoseconds = 1000000000;

[[nodiscard]]
inline Timestamp ConvertTimeSecondsToNanoseconds(double seconds) noexcept
{
    return (Timestamp)(seconds * g_one_sec_in_nanoseconds);
}

[[nodiscard]]
inline Timestamp ConvertTicksToNanoseconds(Timestamp ticks, Frequency frequency) noexcept
{
    return ticks * g_one_sec_in_nanoseconds / frequency;
}

[[nodiscard]]
inline float ConvertFrequencyToTickPeriod(Frequency frequency) noexcept
{
    return (float)(g_one_sec_in_nanoseconds) / static_cast<float>(frequency);
}

#ifdef _WIN32

inline uint64_t GetQpcFrequency() noexcept
{
    LARGE_INTEGER t;
    QueryPerformanceFrequency( &t );
    return static_cast<uint64_t>(t.QuadPart);
}

inline uint64_t GetQpcToNSecMultiplier() noexcept
{
    static const auto s_qpc_to_nsec = static_cast<uint64_t>(static_cast<double>(g_one_sec_in_nanoseconds) / static_cast<double>(GetQpcFrequency()));
    return s_qpc_to_nsec;
}

#else // !defined(_WIN32)

inline uint64_t GetQpcToNSecMultiplier() noexcept
{
    return 1U;
}

#endif // defined(_WIN32)

} // namespace Methane::Data
