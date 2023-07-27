/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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
class RectSize // NOSONAR - class has more than 35 methods
{
public:
    using DimensionType = D;

    static RectSize<D> Max() noexcept { return RectSize(std::numeric_limits<D>::max(), std::numeric_limits<D>::max()); }

    RectSize() = default;

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    RectSize(V w, V h) noexcept(std::is_unsigned_v<V>)
        : m_width(RoundCast<D>(w))
        , m_height(RoundCast<D>(h))
    {
        if constexpr (std::is_signed_v<V>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(w, 0, "rectangle m_width can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(h, 0, "rectangle m_height can not be less than zero");
        }
    }

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    explicit RectSize(const Point2T<V>& point) noexcept(std::is_unsigned_v<V>)
        : m_width(RoundCast<D>(point.GetX()))
        , m_height(RoundCast<D>(point.GetY()))
    {
        if constexpr (std::is_signed_v<V>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(point.GetX(), 0, "rectangle m_width can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(point.GetY(), 0, "rectangle m_height can not be less than zero");
        }
    }

    template<typename V, typename = std::enable_if_t<!std::is_same_v<D, V>>>
    explicit RectSize(const RectSize<V>& other) noexcept
        : RectSize(RoundCast<D>(other.GetWidth()), RoundCast<D>(other.GetHeight()))
    { }

    D GetWidth() const noexcept  { return m_width; }
    D GetHeight() const noexcept { return m_height; }
    
    void SetWidth(D width) noexcept(std::is_unsigned_v<D>)
    {
        if constexpr (std::is_signed_v<D>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(width, 0, "rectangle width can not be less than zero");
        }
        m_width = width;
    }

    void SetHeight(D height) noexcept(std::is_unsigned_v<D>)
    {
        if constexpr (std::is_signed_v<D>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(height, 0, "rectangle height can not be less than zero");
        }
        m_height = height;
    }
    
    D GetPixelsCount() const noexcept { return m_width * m_height; }
    D GetLongestSide() const noexcept { return std::max(m_width, m_height); }

    bool operator==(const RectSize& other) const noexcept
    { return std::tie(m_width, m_height) == std::tie(other.m_width, other.m_height); }

    bool operator!=(const RectSize& other) const noexcept
    { return std::tie(m_width, m_height) != std::tie(other.m_width, other.m_height); }

    bool operator<=(const RectSize& other) const noexcept
    { return m_width <= other.m_width && m_height <= other.m_height; }

    bool operator<(const RectSize& other) const noexcept
    { return m_width < other.m_width && m_height < other.m_height; }

    bool operator>=(const RectSize& other) const noexcept
    { return m_width >= other.m_width && m_height >= other.m_height; }

    bool operator>(const RectSize& other) const noexcept
    { return m_width > other.m_width && m_height > other.m_height; }

    RectSize operator+(const RectSize& other) const noexcept
    { return RectSize(m_width + other.m_width, m_height + other.m_height); }

    RectSize operator-(const RectSize& other) const noexcept
    { return RectSize(m_width - other.m_width, m_height - other.m_height); }

    RectSize& operator+=(const RectSize& other) noexcept
    { m_width += other.m_width; m_height += other.m_height; return *this; }

    RectSize& operator-=(const RectSize& other) noexcept
    { m_width -= other.m_width; m_height -= other.m_height; return *this; }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator*(M multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier, 0, "rectangle size multiplier can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(m_width) * multiplier), RoundCast<D>(static_cast<M>(m_height) * multiplier));
        else
            return RectSize(m_width * RoundCast<D>(multiplier), m_height * RoundCast<D>(multiplier));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator/(M divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_DESCR(divisor, 0, "rectangle size divisor can not be less or equal to zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(m_width) / divisor), RoundCast<D>(static_cast<M>(m_height) / divisor));
        else
            return RectSize(m_width / RoundCast<D>(divisor), m_height / RoundCast<D>(divisor));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator*=(M multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier, 0, "rectangle size multiplier can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            m_width  = RoundCast<D>(static_cast<M>(m_width)  * multiplier);
            m_height = RoundCast<D>(static_cast<M>(m_height) * multiplier);
        }
        else
        {
            m_width  = m_width  * RoundCast<D>(multiplier);
            m_height = m_height * RoundCast<D>(multiplier);
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator/=(M divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_DESCR(divisor, 0, "rectangle size divisor can not be less or equal to zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            m_width  = RoundCast<D>(static_cast<M>(m_width)  / divisor);
            m_height = RoundCast<D>(static_cast<M>(m_height) / divisor);
        }
        else
        {
            m_width  = m_width  / RoundCast<D>(divisor);
            m_height = m_height / RoundCast<D>(divisor);
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator*(const Point2T<M>& multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetX(), 0, "rectangle size multiplier coordinate x can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetY(), 0, "rectangle size multiplier coordinate y can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(m_width) * multiplier.GetX()), RoundCast<D>(static_cast<M>(m_height) * multiplier.GetY()));
        else
            return RectSize(m_width * RoundCast<D>(multiplier.GetX()), m_height * RoundCast<D>(multiplier.GetY()));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator/(const Point2T<M>& divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetX(), 0, "rectangle size divisor coordinate x can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetY(), 0, "rectangle size divisor coordinate y can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(m_width) / divisor.GetX()), RoundCast<D>(static_cast<M>(m_height) / divisor.GetY()));
        else
            return RectSize(m_width / RoundCast<D>(divisor.GetX()), m_height / RoundCast<D>(divisor.GetY()));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator*=(const Point2T<M>& multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetX(), 0, "rectangle size multiplier coordinate x can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetY(), 0, "rectangle size multiplier coordinate y can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            m_width  = RoundCast<D>(static_cast<M>(m_width)  * multiplier.GetX());
            m_height = RoundCast<D>(static_cast<M>(m_height) * multiplier.GetY());
        }
        else
        {
            m_width  = m_width  * RoundCast<D>(multiplier.GetX());
            m_height = m_height * RoundCast<D>(multiplier.GetY());
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator/=(const Point2T<M>& divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetX(), 0, "rectangle size divisor coordinate x can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetY(), 0, "rectangle size divisor coordinate y can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            m_width  = RoundCast<D>(static_cast<M>(m_width)  / divisor.GetX());
            m_height = RoundCast<D>(static_cast<M>(m_height) / divisor.GetY());
        }
        else
        {
            m_width  = m_width  / RoundCast<D>(divisor.GetX());
            m_height = m_height / RoundCast<D>(divisor.GetY());
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator*(const RectSize<M>& multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetWidth(), 0, "rectangle size multiplier width can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetHeight(), 0, "rectangle size multiplier height can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(m_width) * multiplier.GetWidth()), RoundCast<D>(static_cast<M>(m_height) * multiplier.GetHeight()));
        else
            return RectSize(m_width * RoundCast<D>(multiplier.GetWidth()), m_height * RoundCast<D>(multiplier.GetHeight()));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize> operator/(const RectSize<M>& divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetWidth(), 0, "rectangle size divisor width can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetHeight(), 0, "rectangle size divisor height can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            return RectSize(RoundCast<D>(static_cast<M>(m_width) / divisor.GetWidth()), RoundCast<D>(static_cast<M>(m_height) / divisor.GetHeight()));
        else
            return RectSize(m_width / RoundCast<D>(divisor.GetWidth()), m_height / RoundCast<D>(divisor.GetHeight()));
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator*=(const RectSize<M>& multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetWidth(), 0, "rectangle size multiplier width can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetHeight(), 0, "rectangle size multiplier height can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            m_width  = RoundCast<D>(static_cast<M>(m_width)  * multiplier.GetWidth());
            m_height = RoundCast<D>(static_cast<M>(m_height) * multiplier.GetHeight());
        }
        else
        {
            m_width  = m_width  * RoundCast<D>(multiplier.GetWidth());
            m_height = m_height * RoundCast<D>(multiplier.GetHeight());
        }
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, RectSize&> operator/=(const RectSize<M>& divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetWidth(), 0, "rectangle size divisor width can not be less than zero");
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetHeight(), 0, "rectangle size divisor height can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
        {
            m_width  = RoundCast<D>(static_cast<M>(m_width)  / divisor.GetWidth());
            m_height = RoundCast<D>(static_cast<M>(m_height) / divisor.GetHeight());
        }
        else
        {
            m_width  = m_width  / RoundCast<D>(divisor.GetWidth());
            m_height = m_height / RoundCast<D>(divisor.GetHeight());
        }
        return *this;
    }

    explicit operator bool() const noexcept { return m_width && m_height; }

    template<typename V, typename = std::enable_if_t<!std::is_same_v<D, V>>>
    explicit operator RectSize<V>() const noexcept
    {
        return RectSize<V>(RoundCast<V>(m_width), RoundCast<V>(m_height));
    }

    template<typename V>
    explicit operator Point<V,2>() const noexcept
    {
        return Point<V,2>(RoundCast<V>(m_width), RoundCast<V>(m_height));
    }

    explicit operator std::string() const noexcept
    {
        return fmt::format("Sz({} x {})", m_width, m_height);
    }
    
private:
    D m_width  { };
    D m_height { };
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
    Rect(T x, T y, D w, D h) : origin(x, y), size(w, h) { }

    T GetLeft() const noexcept   { return origin.GetX(); }
    T GetRight() const noexcept  { return origin.GetX() + RoundCast<T>(size.GetWidth()); }
    T GetTop() const noexcept    { return origin.GetY(); }
    T GetBottom() const noexcept { return origin.GetY() + RoundCast<T>(size.GetHeight()); }

    bool operator==(const Rect& other) const noexcept
    {
        return std::tie(origin, size) == std::tie(other.origin, other.size);
    }

    bool operator!=(const Rect& other) const noexcept
    {
        return std::tie(origin, size) != std::tie(other.origin, other.size);
    }

    bool operator<(const Rect& other) const noexcept
    {
        return std::tie(origin, size) < std::tie(other.origin, other.size);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Rect<T, D>> operator*(M multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier, 0, "rectangle multiplier can not be less than zero");
        return Rect<T, D>(origin * multiplier, size * multiplier);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Rect<T, D>> operator/(M divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor, 0, "rectangle divisor can not be less than zero");
        return Rect<T, D>(origin / divisor, size / divisor);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Rect<T, D>&> operator*=(M multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier, 0, "rectangle multiplier can not be less than zero");
        origin *= multiplier;
        size   *= multiplier;
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Rect<T, D>&> operator/=(M divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor, 0, "rectangle divisor can not be less than zero");
        origin /= divisor;
        size   /= divisor;
        return *this;
    }

    template<typename V, typename K, typename = std::enable_if_t<!std::is_same_v<T, V> || !std::is_same_v<D, K>>>
    explicit operator Rect<V, K>() const
    {
        return Rect<V, K>(static_cast<Point2T<V>>(origin), static_cast<RectSize<K>>(size));
    }

    explicit operator std::string() const
    {
        return fmt::format("Rect[{} : {}]", static_cast<std::string>(origin), static_cast<std::string>(size));
    }
};

using FrameRect    = Rect<int32_t, uint32_t>;
using FrameSize    = RectSize<uint32_t>;
using FramePoint   = Point2T<int32_t>;

using FloatRect    = Rect<float, float>;
using FloatSize    = RectSize<float>;
using FloatPoint   = Point2T<float>;

} // namespace Methane::Data
