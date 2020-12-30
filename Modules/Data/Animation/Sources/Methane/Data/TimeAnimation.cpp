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

FILE: Methane/Data/TimeAnimation.cpp
Time-based animation of any external entity with an update lambda-function.

******************************************************************************/

#include <Methane/Data/TimeAnimation.h>
#include <Methane/Instrumentation.h>

namespace Methane::Data
{

TimeAnimation::TimeAnimation(const FunctionType& update_function, double duration_sec)
    : Animation(duration_sec)
    , m_update_function(update_function)
{
    META_FUNCTION_TASK();
}

void TimeAnimation::Restart() noexcept
{
    META_FUNCTION_TASK();
    m_prev_elapsed_seconds = 0.0;
    Animation::Restart();
}

bool TimeAnimation::Update()
{
    META_FUNCTION_TASK();
    if (GetState() != State::Running)
        return false;

    const double elapsed_seconds = GetElapsedSecondsD();
    if (IsTimeOver() || !m_update_function(elapsed_seconds, elapsed_seconds - m_prev_elapsed_seconds))
    {
        Stop();
    }
    m_prev_elapsed_seconds = elapsed_seconds;

    return GetState() == State::Running;
}

void TimeAnimation::DryUpdate()
{
    META_FUNCTION_TASK();
    m_update_function(m_prev_elapsed_seconds, 0.0);
}

} // namespace Methane::Data
