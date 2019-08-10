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

FILE: Methane/Graphics/FpsCounter.h
FPS counter calculates frame time duration with moving average window algorithm.

******************************************************************************/

#pragma once

#include <Methane/Data/Timer.h>

#include <cmath>
#include <queue>

namespace Methane
{
namespace Graphics
{

class FpsCounter
{
public:
    FpsCounter() = default;
    FpsCounter(uint32_t averaged_timings_count) : m_averaged_timings_count(averaged_timings_count) {}

    void Reset(uint32_t averaged_timings_count);
    void OnFramePresented();

    inline uint32_t GetAveragedTimingsCount() const noexcept   { return static_cast<uint32_t>(m_frame_timings.size()); }
    inline double   GetAverageFrameTimeSec() const noexcept    { return m_frame_timings_sum / GetAveragedTimingsCount(); }
    inline double   GetAverageFrameTimeMilSec() const noexcept { return GetAverageFrameTimeSec() * 1000; }
    inline uint32_t GetFramesPerSecond() const noexcept        { return static_cast<uint32_t>(std::round(1.0 / GetAverageFrameTimeSec())); }

private:
    Data::Timer        m_frame_timer;
    uint32_t           m_averaged_timings_count = 100;
    double             m_frame_timings_sum = 0.0;
    std::queue<double> m_frame_timings;
};

} // namespace Graphics
} // namespace Methane
