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

FILE: Methane/Data/Point2D.hpp
2D Point type based on cml::vector

******************************************************************************/

#pragma once

#include <cml/vector.h>
#include <string>
#include <cstdint>

namespace Methane::Data
{

template<typename T>
class Point2T : public cml::vector<T, cml::fixed<2>>
{
public:
    using Base = cml::vector<T, cml::fixed<2>>;
    using Base::Base;

    T GetX() const noexcept { return (*this)[0]; }
    T GetY() const noexcept { return (*this)[1]; }

    void SetX(T x) noexcept { (*this)[0] = x; }
    void SetY(T y) noexcept { (*this)[1] = y; }

    template<typename U>
    explicit operator Point2T<U>() const
    { return Point2T<U>(static_cast<U>(GetX()), static_cast<U>(GetY())); }

    operator std::string() const
    { return "Pt(" + std::to_string(GetX()) + ", " + std::to_string(GetY()) + ")"; }
};

using Point2i = Point2T<int32_t>;
using Point2u = Point2T<uint32_t>;
using Point2f = Point2T<float>;
using Point2d = Point2T<double>;

} // namespace Methane::Data
