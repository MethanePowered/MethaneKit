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

namespace Methane::Data
{

using TimeRange = Range<Timestamp>;

constexpr Timestamp g_one_sec_in_nanoseconds = 1000000000;

inline Timestamp ConvertTimeSecondsToNanoseconds(double seconds)
{
    return static_cast<Timestamp>(seconds * g_one_sec_in_nanoseconds);
}

inline Timestamp ConvertTicksToNanoseconds(Timestamp ticks, Frequency frequency)
{
    return static_cast<Timestamp>(ticks * g_one_sec_in_nanoseconds / frequency);
}

inline float ConvertFrequencyToTickPeriod(Frequency frequency)
{
    return static_cast<float>(g_one_sec_in_nanoseconds) / frequency;
}

} // namespace Methane::Data
