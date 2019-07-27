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

FILE: Methane/Data/Animation.hpp
Abstract animation class

******************************************************************************/

#pragma once

#include "Timer.hpp"

#include <functional>

namespace Methane
{
namespace Data
{

template<typename ValueType>
class Animation : public Timer
{
public:
    using FunctionType = std::function<bool(ValueType& value_to_update, const ValueType& start_value, double elapsed_seconds)>;

    Animation(ValueType& value, const FunctionType& update_function)
        : Timer()
        , m_value(value)
        , m_start_value(value)
        , m_update_function(update_function)
    { }

    bool   IsRunning() const                { return m_is_running; }
    double GetDuration() const              { return m_duration_sec; }
    void   SetDuration(double duration_sec) { m_duration_sec = duration_sec; }

    void Reset() noexcept
    {
        m_is_running = true;
        m_start_value = m_value;
        Timer::Reset();
    }

    bool Update()
    {
        if (!m_is_running)
            return false;

        const double elapsed_seconds = GetElapsedSecondsD();
        m_is_running = elapsed_seconds < m_duration_sec && 
                       m_update_function(m_value, m_start_value, elapsed_seconds);

        return m_is_running;
    }

protected:
    using Timer::Reset;

private:
    ValueType&          m_value;
    ValueType           m_start_value;
    const FunctionType  m_update_function;
    double              m_duration_sec      = std::numeric_limits<double>::max();
    bool                m_is_running        = true;
};

} // namespace Data
} // namespace Methane
