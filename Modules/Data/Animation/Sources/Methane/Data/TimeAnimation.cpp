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

FILE: Methane/Data/TimeAnimation.cpp
Time-based animation of any external enity with an update lambda-function.

******************************************************************************/

#include <Methane/Data/TimeAnimation.h>

using namespace Methane::Data;

TimeAnimation::TimeAnimation(const FunctionType& update_function, double duration_sec)
    : Animation(duration_sec)
    , m_update_function(update_function)
{ }

void TimeAnimation::Restart() noexcept
{
    m_prev_elapsed_seconds = 0.0;
    Animation::Restart();
}

bool TimeAnimation::Update()
{
    if (!m_is_running)
        return false;

    const double elapsed_seconds = GetElapsedSecondsD();
    const double delta_seconds = elapsed_seconds - m_prev_elapsed_seconds;
    m_is_running = elapsed_seconds < m_duration_sec && 
                    m_update_function(elapsed_seconds, delta_seconds);
    m_prev_elapsed_seconds = elapsed_seconds;

    return m_is_running;
}