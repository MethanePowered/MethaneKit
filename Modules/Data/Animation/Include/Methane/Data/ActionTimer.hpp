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

FILE: Methane/Data/ActionTimer.hpp
Input action timer for tracking the action active time

******************************************************************************/

#pragma once

#include <chrono>

namespace Methane
{
namespace Data
{

template<typename ActionType>
class ActionTimer
{
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    ActionTimer(ActionType action)
        : m_action(action)
        , m_time(std::chrono::high_resolution_clock::now())
    { }

    bool operator<(const ActionTimer& other) const    { return m_action < other.m_action; }
    bool operator==(const ActionTimer& other) const   { return m_action == other.m_action; }
    bool operator!=(const ActionTimer& other) const   { return !operator==(other); }

private:
    const ActionType m_action;
    const TimePoint  m_time;
};

} // namespace Data
} // namespace Methane
