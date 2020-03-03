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

FILE: Methane/Data/TimeAnimation.h
Time-based animation of any external entity with an update lambda-function.

******************************************************************************/

#pragma once

#include "Animation.h"

#include <functional>

namespace Methane::Data
{

class TimeAnimation : public Animation
{
public:
    using FunctionType = std::function<bool(double elapsed_seconds, double delta_seconds)>;

    TimeAnimation(const FunctionType& update_function, double duration_sec = std::numeric_limits<double>::max());

    // Animation overrides
    void Restart() noexcept override;
    bool Update() override;

private:
    const FunctionType  m_update_function;
    double              m_prev_elapsed_seconds = 0.0;
};

} // namespace Methane::Data
