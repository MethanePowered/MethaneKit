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

FILE: Methane/Data/Rect.hpp
Rectangle type based on point and size

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

    D width  { };
    D height { };

    static RectSize<D> Max() noexcept { return RectSize(std::numeric_limits<D>::max(), std::numeric_limits<D>::max()); }

    RectSize() = default;

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    RectSize(V w, V h) noexcept
        : width(RoundCast<D>(w))
        , height(RoundCast<D>(h))
    { }

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    explicit RectSize(const Point2T<V>& point) noexcept
        : width(RoundCast<D>(point.GetX()))
        , height(RoundCast<D>(point.GetY()))
    { }

    template<typename V, typename = std::enable_if_t<!std::is_same_v<D, V>, void>>
    explicit RectSize(const RectSize<V>& other) noexcept
        : RectSize(static_cast<D>(other.width), static_cast<D>(other.height))
    { }

    D GetPixelsCount() const noexcept { return width * height; }
    D GetLongestSide() const noexcept { return std::max(width, height); }

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

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator*(M multiplier) const noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(width) * multiplier), RoundCast<D>(static_cast<M>(height) * multiplier));
        else
            return RectSize(width * RoundCast<D>(multiplier), height * RoundCast<D>(multiplier));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator/(M divisor) const noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(width) / divisor), RoundCast<D>(static_cast<M>(height) / divisor));
        else
            return RectSize(width / RoundCast<D>(divisor), height / RoundCast<D>(divisor));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator*=(M multiplier) noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            width  = RoundCast<D>(static_cast<M>(width)  * multiplier);
            height = RoundCast<D>(static_cast<M>(height) * multiplier);
        }
        else
        {
            width  = width  * RoundCast<D>(multiplier);
            height = height * RoundCast<D>(multiplier);
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator/=(M divisor) noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            width  = RoundCast<D>(static_cast<M>(width)  / divisor);
            height = RoundCast<D>(static_cast<M>(height) / divisor);
        }
        else
        {
            width  = width  / RoundCast<D>(divisor);
            height = height / RoundCast<D>(divisor);
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator*(const Point2T<M>& multiplier) const noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(width) * multiplier.GetX()), RoundCast<D>(static_cast<M>(height) * multiplier.GetY()));
        else
            return RectSize(width * RoundCast<D>(multiplier.GetX()), height * RoundCast<D>(multiplier.GetY()));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator/(const Point2T<M>& divisor) const noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(width) / divisor.GetX()), RoundCast<D>(static_cast<M>(height) / divisor.GetY()));
        else
            return RectSize(width / RoundCast<D>(divisor.GetX()), height / RoundCast<D>(divisor.GetY()));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator*=(const Point2T<M>& multiplier) noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            width  = RoundCast<D>(static_cast<M>(width)  * multiplier.GetX());
            height = RoundCast<D>(static_cast<M>(height) * multiplier.GetY());
        }
        else
        {
            width  = width  * RoundCast<D>(multiplier.GetX());
            height = height * RoundCast<D>(multiplier.GetY());
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator/=(const Point2T<M>& divisor) noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            width  = RoundCast<D>(static_cast<M>(width)  / divisor.GetX());
            height = RoundCast<D>(static_cast<M>(height) / divisor.GetY());
        }
        else
        {
            width  = width  / RoundCast<D>(divisor.GetX());
            height = height / RoundCast<D>(divisor.GetY());
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator*(const RectSize<M>& multiplier) const noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(width) * multiplier.width), RoundCast<D>(static_cast<M>(height) * multiplier.height));
        else
            return RectSize(width * RoundCast<D>(multiplier.width), height * RoundCast<D>(multiplier.height));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator/(const RectSize<M>& divisor) const noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(width) / divisor.width), RoundCast<D>(static_cast<M>(height) / divisor.height));
        else
            return RectSize(width / RoundCast<D>(divisor.width), height / RoundCast<D>(divisor.height));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator*=(const RectSize<M>& multiplier) noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            width  = RoundCast<D>(static_cast<M>(width)  * multiplier.width);
            height = RoundCast<D>(static_cast<M>(height) * multiplier.height);
        }
        else
        {
            width  = width  * RoundCast<D>(multiplier.width);
            height = height * RoundCast<D>(multiplier.height);
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator/=(const RectSize<M>& divisor) noexcept
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            width  = RoundCast<D>(static_cast<M>(width)  / divisor.width);
            height = RoundCast<D>(static_cast<M>(height) / divisor.height);
        }
        else
        {
            width  = width  / RoundCast<D>(divisor.width);
            height = height / RoundCast<D>(divisor.height);
        }
        return *this;
    }

    explicit operator bool() const noexcept { return width && height; }

    template<typename V>
    explicit operator RectSize<V>() const noexcept
    {
        return RectSize<V>(RoundCast<V>(width), RoundCast<V>(height));
    }

    explicit operator std::string() const
    {
        return fmt::format("Sz({} x {})", width, height);
    }
};

template<typename T, typename D,
         typename = std::enable_if_t<std::is_arithmetic_v<T>>,
         typename = std::enable_if_t<std::is_arithmetic_v<D>>>
struct Rect
{
    using CoordinateType = T;
    using DimensionType  = D;

    using Point = Point2T<T>;
    using Size  = RectSize<D>;

    Point origin;
    Size  size;

    Rect() = default;
    explicit Rect(const Size& size) noexcept : size(size) { }
    explicit Rect(const Point& origin) noexcept : origin(origin) { }
    Rect(const Point& origin, const Size& size) noexcept : origin(origin), size(size) { }

    T GetLeft() const   { return origin.GetX(); }
    T GetRight() const  { return origin.GetX() + size.width; }
    T GetTop() const    { return origin.GetY(); }
    T GetBottom() const { return origin.GetY() + size.height; }

    template<typename U>
    explicit operator Rect<U, U>() const
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

    explicit operator std::string() const
    { return fmt::format("Rt[{} : {}]", origin, size); }
};

using FrameRect    = Rect<int32_t, uint32_t>;
using FrameSize    = RectSize<uint32_t>;
using FramePoint   = Point2T<int32_t>;

using FloatRect    = Rect<float, float>;
using FloatSize    = RectSize<float>;
using FloatPoint   = Point2T<float>;

} // namespace Methane::Data
