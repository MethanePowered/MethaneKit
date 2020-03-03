/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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
        FrameTiming() = default;
        FrameTiming(const FrameTiming&) = default;
        FrameTiming(double total_time_sec, double present_time_sec, double gpu_wait_time_sec);

        double GetTotalTimeSec() const    { return m_total_time_sec; }
        double GetPresentTimeSec() const  { return m_present_time_sec; }
        double GetGpuWaitTimeSec() const  { return m_gpu_wait_time_sec; }
        double GetCpuTimeSec() const      { return m_total_time_sec - m_present_time_sec - m_gpu_wait_time_sec; }

        double GetTotalTimeMSec() const   { return m_total_time_sec * 1000.0; }
        double GetPresentTimeMSec() const { return m_present_time_sec * 1000.0; }
        double GetGpuWaitTimeMSec() const { return m_gpu_wait_time_sec * 1000.0; }
        double GetCpuTimeMSec() const     { return GetCpuTimeSec() * 1000.0; }

        double GetCpuTimePercent() const  { return 100.0 * GetCpuTimeSec() / GetTotalTimeSec(); }

        FrameTiming& operator=(const FrameTiming& other) = default;
        FrameTiming& operator+=(const FrameTiming& other);
        FrameTiming& operator-=(const FrameTiming& other);
        FrameTiming  operator/(double divisor) const;
        FrameTiming  operator*(double multiplier) const;

    private:
        double m_total_time_sec    = 0.0;
        double m_present_time_sec  = 0.0;
        double m_gpu_wait_time_sec = 0.0;
    };

    FpsCounter() = default;
    FpsCounter(uint32_t averaged_timings_count) : m_averaged_timings_count(averaged_timings_count) {}

    void Reset(uint32_t averaged_timings_count);
    void OnGpuFramePresentWait();
    void OnGpuFramePresented();
    void OnCpuFrameReadyToPresent();
    void OnCpuFramePresented();

    uint32_t        GetAveragedTimingsCount() const noexcept { return static_cast<uint32_t>(m_frame_timings.size()); }
    FrameTiming     GetAverageFrameTiming() const noexcept   { return m_frame_timings_sum / GetAveragedTimingsCount(); }
    const uint32_t  GetFramesPerSecond() const noexcept      { return static_cast<uint32_t>(std::round(1.0 / GetAverageFrameTiming().GetTotalTimeSec())); }

private:
    Timer                   m_frame_timer;
    Timer                   m_present_timer;
    double                  m_present_on_gpu_wait_time_sec = 0.0;
    uint32_t                m_averaged_timings_count = 100;
    FrameTiming             m_frame_timings_sum;
    std::queue<FrameTiming> m_frame_timings;
};

} // namespace Methane::Graphics
