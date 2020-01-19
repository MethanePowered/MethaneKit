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

FILE: Methane/Data/AnimationsPool.cpp
Pool of animations for centralized updating, adding and removing in application.

******************************************************************************/

#include <Methane/Data/Animation.h>
#include <Methane/Instrumentation.h>

namespace Methane::Data
{

Animation::Animation(double duration_sec)
    : m_duration_sec(duration_sec)
{
    ITT_FUNCTION_TASK();
}

Animation::~Animation()
{
    ITT_FUNCTION_TASK();
}

void Animation::IncreaseDuration(double duration_sec)
{
    ITT_FUNCTION_TASK();
    m_duration_sec = GetElapsedSecondsD() + duration_sec;
}

void Animation::Restart() noexcept
{
    ITT_FUNCTION_TASK();
    m_is_running = true;
    Timer::Reset();
}

void Animation::Stop() noexcept
{
    ITT_FUNCTION_TASK();
    m_is_running = false;
}

} // namespace Methane::Data
