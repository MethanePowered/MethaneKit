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

FILE: Methane/Graphics/Timer.h
Timer helper allows to get the number of elapsed seconds between measurements.

******************************************************************************/

#pragma once

#include <chrono>

namespace Methane
{
namespace Graphics
{

class Timer
{
public:
    Timer();

    void Reset() noexcept;
    double GetElapsedSeconds() const noexcept;
    float GetElapsedSecondsF() const noexcept;

private:
    std::chrono::high_resolution_clock::time_point m_start_time;
};

} // namespace Graphics
} // namespace Methane
