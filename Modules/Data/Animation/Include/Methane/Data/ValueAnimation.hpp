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

FILE: Methane/Data/ValueAnimation.hpp
Abstract value animation implementation with an update function.

******************************************************************************/

#pragma once

#include "Animation.h"

#include <functional>

namespace Methane
{
namespace Data
{

template<typename ValueType>
class ValueAnimation : public Animation
{
public:
    using FunctionType = std::function<bool(ValueType& value_to_update, const ValueType& start_value,
                                            double elapsed_seconds, double delta_seconds)>;

    ValueAnimation(ValueType& value, const FunctionType& update_function)
        : m_value(value)
        , m_start_value(value)
        , m_update_function(update_function)
    { }

    // Animation overrides

    void Restart() noexcept override
    {
        m_start_value = m_value;
        m_prev_elapsed_seconds = 0.0;
        Animation::Restart();
    }

    bool Update() override
    {
        if (!m_is_running)
            return false;

        const double elapsed_seconds = GetElapsedSecondsD();
        const double delta_seconds = elapsed_seconds - m_prev_elapsed_seconds;
        m_is_running = elapsed_seconds < m_duration_sec && 
                       m_update_function(m_value, m_start_value, elapsed_seconds, delta_seconds);
        m_prev_elapsed_seconds = elapsed_seconds;

        return m_is_running;
    }

private:
    ValueType&          m_value;
    ValueType           m_start_value;
    const FunctionType  m_update_function;
    double              m_prev_elapsed_seconds = 0.0;
};

} // namespace Data
} // namespace Methane
