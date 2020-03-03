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

FILE: Methane/Data/ValueAnimation.hpp
Abstract value animation based on time with an update lambda-function.

******************************************************************************/

#pragma once

#include "Animation.h"

#include <Methane/Instrumentation.h>

#include <functional>

namespace Methane::Data
{

template<typename ValueType>
class ValueAnimation : public Animation
{
public:
    using FunctionType = std::function<bool(ValueType& value_to_update, const ValueType& start_value,
                                            double elapsed_seconds, double delta_seconds)>;

    ValueAnimation(ValueType& value, const FunctionType& update_function,
                   double duration_sec = std::numeric_limits<double>::max())
        : Animation(duration_sec)
        , m_value(value)
        , m_start_value(value)
        , m_update_function(update_function)
    {
        ITT_FUNCTION_TASK();
    }

    // Animation overrides

    void Restart() noexcept override
    {
        ITT_FUNCTION_TASK();
        m_start_value = m_value;
        m_prev_elapsed_seconds = 0.0;
        Animation::Restart();
    }

    bool Update() override
    {
        ITT_FUNCTION_TASK();
        if (GetState() != State::Running)
            return false;

        const double elapsed_seconds = GetElapsedSecondsD();
        const double delta_seconds = elapsed_seconds - m_prev_elapsed_seconds;
        if (IsTimeOver() || !m_update_function(m_value, m_start_value, elapsed_seconds, delta_seconds))
        {
            Stop();
        }
        m_prev_elapsed_seconds = elapsed_seconds;

        return GetState() == State::Running;
    }

private:
    ValueType&          m_value;
    ValueType           m_start_value;
    const FunctionType  m_update_function;
    double              m_prev_elapsed_seconds = 0.0;
};

} // namespace Methane::Data
