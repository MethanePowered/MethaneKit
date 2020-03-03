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

FILE: Methane/Data/Types.h
Common Methane primitive data types

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

template<typename T, typename D>
struct Rect
{
    struct Size
    {
        D width = 0;
        D height = 0;
        
        Size() = default;
        Size(const Size& size) = default;
        Size(D w, D h) : width(w), height(h) { }
        
        bool operator==(const Size& other) const noexcept
        { return std::tie(width, height) == std::tie(other.height, other.width); }

        bool operator!=(const Size& other) const noexcept
        { return std::tie(width, height) != std::tie(other.height, other.width); }

        Size& operator*(D multiplier)
        {
            width *= multiplier; height *= multiplier;
            return *this;
        }

        Size& operator/(D divisor)
        {
            width /= divisor; height /= divisor;
            return *this;
        }

        D GetPixelsCount() const noexcept { return width * height; }
        D GetLongestSide() const noexcept { return std::max(width, height); }

        operator std::string() const
        { return "Sz(" + std::to_string(width) + " x " + std::to_string(height) + ")"; }
    };
    
    template<typename U>
    operator Rect<U, U>() const
    { return { static_cast<Point2T<U>>(origin), static_cast<typename Rect<U, U>::Size>(size) }; }

    bool operator==(const Rect& other) const noexcept
    { return std::tie(origin, size) == std::tie(other.origin, other.size); }

    bool operator!=(const Rect& other) const noexcept
    { return std::tie(origin, size) != std::tie(other.origin, other.size); }

    operator std::string() const
    { return std::string("Rt[") + origin + " + " + size + "]"; }

    using Point = Point2T<T>;

    Point origin;
    Size  size;
};

using FrameRect    = Rect<int32_t, uint32_t>;
using FrameSize    = FrameRect::Size;

using Bytes = std::vector<uint8_t>;
using Size = uint32_t;
using Index = Size;
using RawPtr = uint8_t*;
using ConstRawPtr = const uint8_t* const;

} // namespace Methane::Data
