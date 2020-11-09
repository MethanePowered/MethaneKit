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

FILE: Methane/Graphics/FpsCounter.h
FPS counter calculates frame time duration with moving average window algorithm.

******************************************************************************/

#pragma once

#include <Methane/Timer.hpp>

#include <cmath>
#include <queue>

namespace Methane::Graphics
{

class FpsCounter
{
public:
    class FrameTiming
    {
    public:
        FrameTiming() noexcept = default;
        FrameTiming(const FrameTiming&) noexcept = default;
        FrameTiming(double total_time_sec, double present_time_sec, double gpu_wait_time_sec) noexcept;

        double GetTotalTimeSec() const noexcept   { return m_total_time_sec; }
        double GetPresentTimeSec() const noexcept { return m_present_time_sec; }
        double GetGpuWaitTimeSec() const noexcept { return m_gpu_wait_time_sec; }
        double GetCpuTimeSec() const noexcept     { return m_total_time_sec - m_present_time_sec - m_gpu_wait_time_sec; }

        double GetTotalTimeMSec() const noexcept  { return m_total_time_sec * 1000.0; }
        double GetPresentTimeMSec() const noexcept{ return m_present_time_sec * 1000.0; }
        double GetGpuWaitTimeMSec() const noexcept{ return m_gpu_wait_time_sec * 1000.0; }
        double GetCpuTimeMSec() const noexcept    { return GetCpuTimeSec() * 1000.0; }

        double GetCpuTimePercent() const noexcept { return 100.0 * GetCpuTimeSec() / GetTotalTimeSec(); }

        FrameTiming& operator=(const FrameTiming& other) noexcept = default;
        FrameTiming& operator+=(const FrameTiming& other) noexcept;
        FrameTiming& operator-=(const FrameTiming& other) noexcept;
        FrameTiming  operator/(double divisor) const noexcept;
        FrameTiming  operator*(double multiplier) const noexcept;

    private:
        double m_total_time_sec    = 0.0;
        double m_present_time_sec  = 0.0;
        double m_gpu_wait_time_sec = 0.0;
    };

    FpsCounter() = default;
    FpsCounter(uint32_t averaged_timings_count) noexcept : m_averaged_timings_count(averaged_timings_count) { }

    void Reset(uint32_t averaged_timings_count) noexcept;
    void OnGpuFramePresentWait() noexcept    { m_present_timer.Reset(); }
    void OnCpuFrameReadyToPresent() noexcept { m_present_timer.Reset(); }
    void OnGpuFramePresented() noexcept      { m_present_on_gpu_wait_time_sec = m_present_timer.GetElapsedSecondsD(); }
    void OnCpuFramePresented() noexcept;

    uint32_t    GetAveragedTimingsCount() const noexcept { return static_cast<uint32_t>(m_frame_timings.size()); }
    FrameTiming GetAverageFrameTiming() const noexcept;
    uint32_t    GetFramesPerSecond() const noexcept;

private:
    void ResetPresentTimer() noexcept;

    Timer                   m_frame_timer;
    Timer                   m_present_timer;
    double                  m_present_on_gpu_wait_time_sec = 0.0;
    uint32_t                m_averaged_timings_count = 100;
    FrameTiming             m_frame_timings_sum;
    std::queue<FrameTiming> m_frame_timings;
};

} // namespace Methane::Graphics
