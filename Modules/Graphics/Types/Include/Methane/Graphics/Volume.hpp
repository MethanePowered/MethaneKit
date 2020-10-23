/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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
        explicit Size(const typename Rect<T, D>::Size& rect_size, D d = 1) : Rect<T, D>::Size(rect_size), depth(d) { }
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

        template<typename M>
        Size operator*(M multiplier) const noexcept
        { return Size(RectSize::operator*(multiplier), static_cast<D>(static_cast<M>(depth) * multiplier)); }

        template<typename M>
        Size operator/(M divisor) const noexcept
        { return Size(RectSize::operator/(divisor), static_cast<D>(static_cast<M>(depth) / divisor)); }

        Size& operator+=(const Size& other) noexcept
        { depth += other.depth; return RectSize::operator+=(other); }

        Size& operator-=(const Size& other) noexcept
        { depth -= other.depth; return RectSize::operator-=(other); }

        template<typename M>
        Size& operator*=(M multiplier) noexcept
        {
            depth = static_cast<D>(static_cast<M>(depth) * multiplier);
            return RectSize::operator*=(multiplier);
        }

        template<typename M>
        Size& operator/=(M divisor) noexcept
        {
            depth = static_cast<D>(static_cast<M>(depth) / divisor);
            return RectSize::operator/=(divisor);
        }

        operator bool() const noexcept
        { return depth && RectSize::operator bool(); }

        D GetPixelsCount() const noexcept { return depth * Rect<T, D>::Size::GetPixelsCount(); }
        D GetLongestSide() const noexcept { return std::max(depth, Rect<T, D>::Size::GetLongestSide()); }

        operator std::string() const
        {
            std::string result = "Sz(" + std::to_string(Rect<T, D>::Size::width);
            if (Rect<T, D>::Size::height != 1 || depth != 1)
                result += " x " + std::to_string(Rect<T, D>::Size::height);
            if (depth != 1)
                result += " x " + std::to_string(depth);
            result += ")";
            return result;
        }
    };

    bool operator==(const Volume& other) const noexcept
    { return std::tie(origin, size) == std::tie(other.origin, other.size); }

    bool operator!=(const Volume& other) const noexcept
    { return std::tie(origin, size) != std::tie(other.origin, other.size); }

    template<typename M>
    Rect<T, D> operator*(M multiplier) const noexcept
    { return Rect<T, D>{ origin* multiplier, size* multiplier }; }

    template<typename M>
    Rect<T, D> operator/(M divisor) const noexcept
    { return Rect<T, D>{ origin / divisor, size / divisor }; }

    template<typename M>
    Rect<T, D>& operator*=(M multiplier) noexcept
    { origin *= multiplier; size *= multiplier; return *this; }

    template<typename M>
    Rect<T, D>& operator/=(M divisor) noexcept
    { origin /= divisor; size /= divisor; return *this; }

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
