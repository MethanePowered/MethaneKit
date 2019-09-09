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

FILE: Methane/Data/Timer.h
Basic animation timer for measuring elapsed time since start.

******************************************************************************/

#pragma once

#include <Methane/Instrumentation.h>

#include <chrono>

namespace Methane::Data
{

class Timer
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    Timer() : m_start_time(Clock::now()) { ITT_FUNCTION_TASK(); }

    void Reset() noexcept
    {
        ITT_FUNCTION_TASK();
        m_start_time = Clock::now();
    }

    TimePoint   GetStartTime() const noexcept       { return m_start_time; }
    uint32_t    GetElapsedSecondsU() const noexcept { return GetElapsedSeconds<uint32_t>(); }
    double      GetElapsedSecondsD() const noexcept { return GetElapsedSeconds<double>(); }
    float       GetElapsedSecondsF() const noexcept { return GetElapsedSeconds<float>(); }

    template<typename T>
    T GetElapsedSeconds() const noexcept
    {
        ITT_FUNCTION_TASK();
        return std::chrono::duration_cast<std::chrono::duration<T>>(Clock::now() - m_start_time).count();
    }

private:
    TimePoint m_start_time;
};

} // namespace Methane::Data
