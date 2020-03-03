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

FILE: Methane/Data/AnimationsPool.h
Pool of animations for centralized updating, adding and removing in application.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#include "Animation.h"

#include <deque>

namespace Methane::Data
{

using Animations = std::deque<Ptr<Animation>>;

class AnimationsPool : public Animations
{
public:
    void Update();
    void Pause();
    void Resume();

private:
    bool m_is_paused = false;
};

} // namespace Methane::Data
