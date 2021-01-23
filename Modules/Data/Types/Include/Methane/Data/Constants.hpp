/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Data/Constants.hpp
Floating point math constants

******************************************************************************/

#pragma once

#include <type_traits>
#include <cmath>

namespace Methane::Data
{

template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
struct Constants
{
    static constexpr T Sqrt2     = static_cast<T>(1.41421356237309504880168872420969808);
    static constexpr T E         = static_cast<T>(2.71828182845904523536028747135266250);
    static constexpr T Pi        = static_cast<T>(3.14159265358979323846264338327950288);
    static constexpr T InvPi     = T(1) / Pi;
    static constexpr T TwoPi     = Pi * T(2);
    static constexpr T PiDiv2    = Pi / T(2);
    static constexpr T DegPerRad = T(180) / Pi;
    static constexpr T RadPerDeg = Pi / T(180);

    Constants() = delete;
};

using ConstFloat  = Constants<float>;
using ConstDouble = Constants<double>;

} // namespace Methane::Data