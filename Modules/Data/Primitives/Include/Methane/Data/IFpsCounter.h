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

FILE: Methane/Data/IFpsCounter.h
FPS counter interface.

******************************************************************************/

#pragma once

#include <cstdint>

namespace Methane::Data
{

class FrameTiming
{
public:
    FrameTiming() = default;
    FrameTiming(const FrameTiming&) noexcept = default;
    FrameTiming(double total_time_sec, double present_time_sec, double gpu_wait_time_sec) noexcept;

    [[nodiscard]] double GetTotalTimeSec() const noexcept   { return m_total_time_sec; }
    [[nodiscard]] double GetPresentTimeSec() const noexcept { return m_present_time_sec; }
    [[nodiscard]] double GetGpuWaitTimeSec() const noexcept { return m_gpu_wait_time_sec; }
    [[nodiscard]] double GetCpuTimeSec() const noexcept     { return m_total_time_sec - m_present_time_sec - m_gpu_wait_time_sec; }

    [[nodiscard]] double GetTotalTimeMSec() const noexcept  { return m_total_time_sec * 1000.0; }
    [[nodiscard]] double GetPresentTimeMSec() const noexcept{ return m_present_time_sec * 1000.0; }
    [[nodiscard]] double GetGpuWaitTimeMSec() const noexcept{ return m_gpu_wait_time_sec * 1000.0; }
    [[nodiscard]] double GetCpuTimeMSec() const noexcept    { return GetCpuTimeSec() * 1000.0; }

    [[nodiscard]] double GetCpuTimePercent() const noexcept { return 100.0 * GetCpuTimeSec() / GetTotalTimeSec(); }

    FrameTiming& operator=(const FrameTiming& other) noexcept = default;
    FrameTiming& operator+=(const FrameTiming& other) noexcept;
    FrameTiming& operator-=(const FrameTiming& other) noexcept;
    FrameTiming  operator/(double divisor) const noexcept;
    FrameTiming  operator*(double multiplier) const noexcept;

private:
    double m_total_time_sec    { 0.0 };
    double m_present_time_sec  { 0.0 };
    double m_gpu_wait_time_sec { 0.0 };
};

class IFpsCounter
{
public:
    using Timing = FrameTiming;

    virtual void Reset(uint32_t averaged_timings_count) noexcept = 0;
    [[nodiscard]] virtual uint32_t GetAveragedTimingsCount() const noexcept = 0;
    [[nodiscard]] virtual Timing   GetAverageFrameTiming() const noexcept = 0;
    [[nodiscard]] virtual uint32_t GetFramesPerSecond() const noexcept = 0;

    virtual ~IFpsCounter() = default;
};

} // namespace Methane::Graphics::Rhi
