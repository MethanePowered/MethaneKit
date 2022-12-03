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

FILE: Methane/Graphics/Base/FpsCounter.h
FPS counter interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IFpsCounter.h>

#include <Methane/Timer.hpp>

#include <queue>

namespace Methane::Graphics::Base
{

class FpsCounter
    : public Rhi::IFpsCounter
{
public:
    FpsCounter() = default;
    explicit FpsCounter(uint32_t averaged_timings_count) noexcept;

    void Reset(uint32_t averaged_timings_count) noexcept override;
    [[nodiscard]] uint32_t GetAveragedTimingsCount() const noexcept override;
    [[nodiscard]] Timing   GetAverageFrameTiming() const noexcept override;
    [[nodiscard]] uint32_t GetFramesPerSecond() const noexcept override;

    void OnGpuFramePresentWait() noexcept;
    void OnCpuFrameReadyToPresent() noexcept;
    void OnGpuFramePresented() noexcept;
    void OnCpuFramePresented() noexcept;

private:
    void ResetPresentTimer() noexcept;

    Timer              m_frame_timer;
    Timer              m_present_timer;
    double             m_present_on_gpu_wait_time_sec = 0.0;
    uint32_t           m_averaged_timings_count = 100;
    Timing             m_frame_timings_sum;
    std::queue<Timing> m_frame_timings;
};

} // namespace Methane::Graphics::Base