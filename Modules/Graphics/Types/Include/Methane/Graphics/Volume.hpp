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

FILE: Methane/Graphics/Volume.hpp
3D Volume type based on cml::vector

******************************************************************************/

#pragma once

#include "Rect.hpp"

#include <Methane/Data/Rect.hpp>

namespace Methane::Graphics
{

template<typename T, typename D>
struct Volume
{
    struct Size : Rect<T, D>::Size
    {
        using RectSize = typename Rect<T, D>::Size;

        D depth = 1;

        Size() = default;
        Size(const typename Rect<T, D>::Size& rect_size, D d = 1) : Rect<T, D>::Size(rect_size), depth(d) { }
        Size(D w, D h, D d = 1) : Rect<T, D>::Size(w, h), depth(d) { }

        bool operator==(const Size& other) const noexcept
        { return RectSize::operator==(other) && depth == other.depth; }

        bool operator!=(const Size& other) const noexcept
        { return RectSize::operator!=(other) || depth != other.depth; }

        bool operator<=(const Size& other) const noexcept
        { return RectSize::operator<=(other) && depth <= other.depth; }

        bool operator<(const Size& other) const noexcept
        { return RectSize::operator<(other) && depth < other.depth; }

        bool operator>=(const Size& other) const noexcept
        { return RectSize::operator>=(other) && depth >= other.depth; }

        bool operator>(const Size& other) const noexcept
        { return RectSize::operator>(other) && depth > other.depth; }

        Size operator+(const Size& other) const noexcept
        { return Size(RectSize::operator+(other), depth + other.depth); }

        Size operator-(const Size& other) const noexcept
        { return Size(RectSize::operator-(other), depth - other.depth); }

        Size operator*(D multiplier) const noexcept
        { return Size(RectSize::operator*(multiplier), depth * multiplier); }

        Size operator/(D divisor) const noexcept
        { return Size(RectSize::operator/(divisor), depth / divisor); }

        Size& operator+=(const Size& other) noexcept
        { depth += other.depth; return RectSize::operator+=(other); }

        Size& operator-=(const Size& other) noexcept
        { depth -= other.depth; return RectSize::operator-=(other); }

        Size& operator*=(D multiplier) noexcept
        { depth *= multiplier; return RectSize::operator*=(multiplier); }

        Size& operator/=(D divisor) noexcept
        { depth /= divisor; return RectSize::operator/=(divisor); }

        operator bool() const noexcept
        { return depth && RectSize::operator bool(); }

        D GetPixelsCount() const noexcept { return depth * Rect<T, D>::Size::GetPixelsCount(); }
        D GetLongestSide() const noexcept { return std::max(depth, Rect<T, D>::Size::GetLongestSide()); }

        operator std::string() const
        {
            return "Sz(" + std::to_string(Rect<T, D>::Size::width) +
                   " x " + std::to_string(Rect<T, D>::Size::height) +
                   " x " + std::to_string(depth) + ")";
        }
    };

    bool operator==(const Volume& other) const noexcept
    { return std::tie(origin, size) == std::tie(other.origin, other.size); }

    bool operator!=(const Volume& other) const noexcept
    { return std::tie(origin, size) != std::tie(other.origin, other.size); }

    operator std::string() const
    { return std::string("Vm[") + origin + " + " + size + "]"; }

    using Point = Point3T<T>;

    Point origin;
    Size  size;
};

using Dimensions = Volume<int32_t, uint32_t>::Size;

using Viewport  = Volume<double, double>;
using Viewports = std::vector<Viewport>;

Viewport GetFrameViewport(const FrameSize& frame_size);
Viewport GetFrameViewport(const FrameRect& frame_rect);

} // namespace Methane::Graphics
