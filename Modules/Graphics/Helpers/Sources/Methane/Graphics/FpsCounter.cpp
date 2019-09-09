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

FILE: Methane/Graphics/FpsCounter.cpp
FPS counter calculates frame time duration with moving average window algorithm.

******************************************************************************/

#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

void FpsCounter::Reset(uint32_t averaged_timings_count)
{
    ITT_FUNCTION_TASK();
    m_averaged_timings_count = averaged_timings_count;
    while (!m_frame_timings.empty())
    {
        m_frame_timings.pop();
    }
    m_frame_timings_sum = 0;
    m_frame_timer.Reset();
}

void FpsCounter::OnFramePresented()
{
    ITT_FUNCTION_TASK();
    if (m_frame_timings.size() >= m_averaged_timings_count)
    {
        m_frame_timings_sum -= m_frame_timings.front();
        m_frame_timings.pop();
    }

    const double frame_seconds = m_frame_timer.GetElapsedSecondsD();
    m_frame_timings_sum += frame_seconds;
    m_frame_timings.push(frame_seconds);

    m_frame_timer.Reset();
}

} // namespace Methane::Graphics
