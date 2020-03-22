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

#include "Point.hpp"

namespace Methane::Data
{

template<typename T, typename D>
struct Rect
{
    struct Size
    {
        D width = 0;
        D height = 0;

        Size() noexcept = default;
        Size(const Size& size) noexcept = default;
        Size(D w, D h) noexcept : width(w), height(h) { }

        template<typename U, typename V>
        Size(const typename Rect<U,V>::Size& other) noexcept
            : Size(static_cast<D>(other.width), static_cast<D>(other.height))
        { }

        bool operator==(const Size& other) const noexcept
        { return std::tie(width, height) == std::tie(other.width, other.height); }

        bool operator!=(const Size& other) const noexcept
        { return std::tie(width, height) != std::tie(other.width, other.height); }

        bool operator<=(const Size& other) const noexcept
        { return width <= other.width && height <= other.height; }

        bool operator<(const Size& other) const noexcept
        { return width < other.width && height < other.height; }

        bool operator>=(const Size& other) const noexcept
        { return width >= other.width && height >= other.height; }

        bool operator>(const Size& other) const noexcept
        { return width > other.width && height > other.height; }

        Size operator+(const Size& other) const noexcept
        { return Size(width + other.width, height + other.height); }

        Size operator-(const Size& other) const noexcept
        { return Size(width - other.width, height - other.height); }

        Size operator*(D multiplier) const noexcept
        { return Size(width * multiplier, height * multiplier); }

        Size operator/(D divisor) const noexcept
        { return Size(width / divisor, height / divisor); }

        Size& operator+=(const Size& other) noexcept
        { width += other.width; height += other.height; return *this; }

        Size& operator-=(const Size& other) noexcept
        { width -= other.width; height -= other.height; return *this; }

        Size& operator*=(D multiplier) noexcept
        { width *= multiplier; height *= multiplier; return *this; }

        Size& operator/=(D divisor) noexcept
        { width /= divisor; height /= divisor; return *this; }

        operator bool() const noexcept
        { return width && height; }

        template<typename U, typename V>
        explicit operator typename Rect<U,V>::Size() const noexcept
        { return Rect<U,V>::Size(static_cast<V>(width), static_cast<V>(height)); }

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

    T GetLeft() const   { return origin.GetX(); }
    T GetRight() const  { return origin.GetX() + size.width; }
    T GetTop() const    { return origin.GetY(); }
    T GetBottom() const { return origin.GetY() + size.height; }

    using Point = Point2T<T>;

    Point origin;
    Size  size;
};

using FrameRect    = Rect<int32_t, uint32_t>;
using FrameSize    = FrameRect::Size;

using FloatRect    = Rect<float, float>;
using FRectSize    = FloatRect::Size;

} // namespace Methane::Data
