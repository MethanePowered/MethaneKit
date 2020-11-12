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

FILE: Methane/Data/Types.h
Common Methane primitive data types

******************************************************************************/

#pragma once

#include "Point.hpp"

#include <fmt/format.h>

namespace Methane::Data
{

template<typename D, typename = std::enable_if_t<std::is_arithmetic_v<D>>>
struct RectSize
{
    using DimensionType = D;

    D width = 0;
    D height = 0;

    RectSize() noexcept = default;
    RectSize(const RectSize& size) noexcept = default;
    RectSize(D w, D h) noexcept : width(w), height(h) { }

    static RectSize<D> Max() noexcept { return RectSize(std::numeric_limits<D>::max(), std::numeric_limits<D>::max()); }

    template<typename V>
    RectSize(const Point2T<V>& point) noexcept : width(static_cast<D>(point.GetX())), height(static_cast<D>(point.GetY())) { }

    template<typename V>
    RectSize(const RectSize<V>& other) noexcept
        : RectSize(static_cast<D>(other.width), static_cast<D>(other.height))
    { }

    bool operator==(const RectSize& other) const noexcept
    { return std::tie(width, height) == std::tie(other.width, other.height); }

    bool operator!=(const RectSize& other) const noexcept
    { return std::tie(width, height) != std::tie(other.width, other.height); }

    bool operator<=(const RectSize& other) const noexcept
    { return width <= other.width && height <= other.height; }

    bool operator<(const RectSize& other) const noexcept
    { return width < other.width && height < other.height; }

    bool operator>=(const RectSize& other) const noexcept
    { return width >= other.width && height >= other.height; }

    bool operator>(const RectSize& other) const noexcept
    { return width > other.width && height > other.height; }

    RectSize operator+(const RectSize& other) const noexcept
    { return RectSize(width + other.width, height + other.height); }

    RectSize operator-(const RectSize& other) const noexcept
    { return RectSize(width - other.width, height - other.height); }

    RectSize& operator+=(const RectSize& other) noexcept
    { width += other.width; height += other.height; return *this; }

    RectSize& operator-=(const RectSize& other) noexcept
    { width -= other.width; height -= other.height; return *this; }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    RectSize operator*(M multiplier) const noexcept
    { return RectSize(static_cast<D>(static_cast<M>(width) * multiplier), static_cast<D>(static_cast<M>(height) * multiplier)); }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    RectSize operator/(M divisor) const noexcept
    { return RectSize(static_cast<D>(static_cast<M>(width) / divisor), static_cast<D>(static_cast<M>(height) / divisor)); }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    RectSize& operator*=(M multiplier) noexcept
    {
        width  = static_cast<D>(static_cast<M>(width) * multiplier);
        height = static_cast<D>(static_cast<M>(height) * multiplier);
        return *this;
    }

    template<typename M, typename = std::enable_if_t<std::is_arithmetic_v<M>>>
    RectSize& operator/=(M divisor) noexcept
    {
        width  = static_cast<D>(static_cast<M>(width) / divisor);
        height = static_cast<D>(static_cast<M>(height) / divisor);
        return *this;
    }

    template<typename M>
    RectSize operator*(const Point2T<M>& multiplier) const noexcept
    { return RectSize(static_cast<D>(static_cast<M>(width) * multiplier.GetX()), static_cast<D>(static_cast<M>(height) * multiplier.GetY())); }

    template<typename M>
    RectSize operator/(const Point2T<M>& divisor) const noexcept
    { return RectSize(static_cast<D>(static_cast<M>(width) / divisor.GetX()), static_cast<D>(static_cast<M>(height) / divisor.GetY())); }

    template<typename M>
    RectSize& operator*=(const Point2T<M>& multiplier) noexcept
    {
        width  = static_cast<D>(static_cast<M>(width) * multiplier.GetX());
        height = static_cast<D>(static_cast<M>(height) * multiplier.GetY());
        return *this;
    }

    template<typename M>
    RectSize& operator/=(const Point2T<M>& divisor) noexcept
    {
        SetX(static_cast<D>(static_cast<M>(width) / divisor.GetX()));
        SetY(static_cast<D>(static_cast<M>(height) / divisor.GetY()));
        return *this;
    }

    template<typename M>
    RectSize operator*(const RectSize<M>& multiplier) const noexcept
    { return RectSize(static_cast<D>(static_cast<M>(width) * multiplier.width), static_cast<D>(static_cast<M>(height) * multiplier.height)); }

    template<typename M>
    RectSize operator/(const RectSize<M>& divisor) const noexcept
    { return RectSize(static_cast<D>(static_cast<M>(width) / divisor.width), static_cast<D>(static_cast<M>(height) / divisor.height)); }

    template<typename M>
    RectSize& operator*=(const RectSize<M>& multiplier) noexcept
    {
        width  = static_cast<D>(static_cast<M>(width)  * multiplier.width);
        height = static_cast<D>(static_cast<M>(height) * multiplier.height);
        return *this;
    }

    template<typename M>
    RectSize& operator/=(const RectSize<M>& divisor) noexcept
    {
        width  = static_cast<D>(static_cast<M>(width)  / divisor.width);
        height = static_cast<D>(static_cast<M>(height) / divisor.height);
        return *this;
    }

    operator bool() const noexcept
    { return width && height; }

    template<typename V>
    explicit operator RectSize<V>() const noexcept
    { return RectSize<V>(static_cast<V>(width), static_cast<V>(height)); }

    D GetPixelsCount() const noexcept { return width * height; }
    D GetLongestSide() const noexcept { return std::max(width, height); }

    operator std::string() const
    { return fmt::format("Sz({} x {})", width, height); }
};

template<typename T, typename D,
         typename = std::enable_if_t<std::is_arithmetic_v<T>>,
         typename = std::enable_if_t<std::is_arithmetic_v<D>>>
struct Rect
{
    using CoordinateType = T;
    using DimensionType  = D;

    template<typename U>
    operator Rect<U, U>() const
    { return { static_cast<Point2T<U>>(origin), static_cast<typename Rect<U, U>::RectSize>(size) }; }

    bool operator==(const Rect& other) const noexcept
    { return std::tie(origin, size) == std::tie(other.origin, other.size); }

    bool operator!=(const Rect& other) const noexcept
    { return std::tie(origin, size) != std::tie(other.origin, other.size); }

    template<typename M>
    Rect<T, D> operator*(M multiplier) const noexcept
    { return Rect<T, D>{ origin * multiplier, size * multiplier }; }

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
    { return fmt::format("Rt[{} + {}]", origin, size); }

    T GetLeft() const   { return origin.GetX(); }
    T GetRight() const  { return origin.GetX() + size.width; }
    T GetTop() const    { return origin.GetY(); }
    T GetBottom() const { return origin.GetY() + size.height; }

    using Point = Point2T<T>;
    using Size  = RectSize<D>;

    explicit Rect(const Size& size) noexcept : size(size) { }
    Rect(const Point& origin, const Size& size) noexcept : origin(origin), size(size) { }
    Rect(const Rect&) noexcept = default;
    Rect() noexcept = default;

    Point origin;
    Size  size;
};

using FrameRect    = Rect<int32_t, uint32_t>;
using FrameSize    = RectSize<uint32_t>;
using FramePoint   = Point2T<int32_t>;

using FloatRect    = Rect<float, float>;
using FloatSize    = RectSize<float>;
using FloatPoint   = Point2T<float>;

} // namespace Methane::Data
