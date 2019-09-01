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

FILE: Methane/Data/Animation.h
Abstract animation class

******************************************************************************/

#pragma once

#include "Timer.h"

#include <memory>

namespace Methane
{
namespace Data
{

class Animation : public Timer
{
public:
    using Ptr     = std::shared_ptr<Animation>;
    using WeakPtr = std::weak_ptr<Animation>;

    Animation(double duration_sec = std::numeric_limits<double>::max());
    virtual ~Animation();

    bool   IsRunning() const noexcept            { return m_is_running; }
    double GetDuration() const noexcept          { return m_duration_sec; }
    void   SetDuration(double duration_sec)      { m_duration_sec = duration_sec; }
    void   IncreaseDuration(double duration_sec);

    virtual void Restart() noexcept;
    virtual void Stop() noexcept;
    virtual bool Update() = 0;

protected:
    using Timer::Reset;

    bool    m_is_running    = true;
    double  m_duration_sec  = std::numeric_limits<double>::max();
};

} // namespace Data
} // namespace Methane
