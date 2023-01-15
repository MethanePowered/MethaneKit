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

FILE: Methane/Graphics/FpsCounter.cpp
FPS counter calculates frame time duration with moving average window algorithm.

******************************************************************************/

#include <Methane/Graphics/Base/FpsCounter.h>

#include <Methane/Instrumentation.h>

#include <cmath>

namespace Methane::Graphics::Base
{

FpsCounter::FpsCounter(uint32_t averaged_timings_count) noexcept
    : m_averaged_timings_count(averaged_timings_count)
{ }

void FpsCounter::Reset(uint32_t averaged_timings_count) noexcept
{
    META_FUNCTION_TASK();
    m_averaged_timings_count = averaged_timings_count;
    while (!m_frame_timings.empty())
    {
        m_frame_timings.pop();
    }
    m_frame_timings_sum = Timing();
    m_present_on_gpu_wait_time_sec = 0.0;
    m_frame_timer.Reset();
    m_present_timer.Reset();
}

void FpsCounter::OnGpuFramePresentWait() noexcept
{
    META_FUNCTION_TASK();
    m_present_timer.Reset();
}

void FpsCounter::OnCpuFrameReadyToPresent() noexcept // NOSONAR - implementation is intentionally identical to OnGpuFramePresentWait()
{
    META_FUNCTION_TASK();
    m_present_timer.Reset();
}

void FpsCounter::OnGpuFramePresented() noexcept
{
    META_FUNCTION_TASK();
    m_present_on_gpu_wait_time_sec = m_present_timer.GetElapsedSecondsD();
}

uint32_t FpsCounter::GetAveragedTimingsCount() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<uint32_t>(m_frame_timings.size());
}

FpsCounter::Timing FpsCounter::GetAverageFrameTiming() const noexcept
{
    META_FUNCTION_TASK();
    const uint32_t averaged_timings_count = GetAveragedTimingsCount();
    return averaged_timings_count ? m_frame_timings_sum / averaged_timings_count : Timing();
}

uint32_t FpsCounter::GetFramesPerSecond() const noexcept
{
    META_FUNCTION_TASK();
    double average_frame_time_sec = GetAverageFrameTiming().GetTotalTimeSec();
    return average_frame_time_sec > 0.0 ? static_cast<uint32_t>(std::round(1.0 / average_frame_time_sec)) : 0U;
}

void FpsCounter::OnCpuFramePresented() noexcept
{
    META_FUNCTION_TASK();
    if (m_frame_timings.size() >= m_averaged_timings_count)
    {
        m_frame_timings_sum -= m_frame_timings.front();
        m_frame_timings.pop();
    }

    const Timing frame_timing(m_frame_timer.GetElapsedSecondsD(),
                              m_present_timer.GetElapsedSecondsD(),
                              m_present_on_gpu_wait_time_sec);

    m_frame_timings_sum += frame_timing;
    m_frame_timings.push(frame_timing);
    m_frame_timer.Reset();
}

} // namespace Methane::Graphics::Base
