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

FILE: Methane/Graphics/Point.hpp
3D Point type based on cml::vector

******************************************************************************/

#pragma once

#include <Methane/Data/Point.hpp>

namespace Methane::Graphics
{

template<typename T>
using Point2T = Data::Point2T<T>;

using Point2i = Point2T<int32_t>;
using Point2u = Point2T<uint32_t>;
using Point2f = Point2T<float>;
using Point2d = Point2T<double>;

template<typename T>
class Point3T : public cml::vector<T, cml::fixed<3>>
{
public:
    Point3T() = default;

    Point3T(T x, T y, T z) : cml::vector<T, cml::fixed<3>>(x, y, z)
    { }

    explicit Point3T(const cml::vector<T, cml::fixed<3>>& v) : cml::vector<T, cml::fixed<3>>(v)
    { }

    T GetX() const noexcept
    { return (*this)[0]; }

    T GetY() const noexcept
    { return (*this)[1]; }

    T GetZ() const noexcept
    { return (*this)[2]; }

    void SetX(T x) noexcept
    { (*this)[0] = x; }

    void SetY(T y) noexcept
    { (*this)[1] = y; }

    void SetZ(T z) noexcept
    { (*this)[2] = z; }

    operator std::string() const
    { return "Pt(" + std::to_string(GetX()) + ", " + std::to_string(GetY()) + ", " + std::to_string(GetZ()) + ")"; }
};

using Point3i = Point3T<int32_t>;
using Point3u = Point3T<uint32_t>;
using Point3f = Point3T<float>;
using Point3d = Point3T<double>;

} // namespace Methane::Graphics