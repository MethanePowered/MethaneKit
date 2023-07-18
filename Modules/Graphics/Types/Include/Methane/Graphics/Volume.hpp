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
3D Volume type based on 3D point and volume size

******************************************************************************/

#pragma once

#include "Rect.hpp"

#include <Methane/Data/Rect.hpp>

#include <fmt/format.h>
#include <sstream>

namespace Methane::Graphics
{

template<typename D>
using RectSize = Data::RectSize<D>;

template<typename D>
class VolumeSize // NOSONAR - class has more than 35 methods
    : public RectSize<D>
{
public:
    static VolumeSize<D> Max() noexcept
    { return VolumeSize(std::numeric_limits<D>::max(), std::numeric_limits<D>::max(), std::numeric_limits<D>::max()); }

    VolumeSize() = default;

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    explicit VolumeSize(const RectSize<V>& rect_size, V d = 1) noexcept(std::is_unsigned_v<V>)
        : RectSize<D>(rect_size)
        , m_depth(Data::RoundCast<D>(d))
    {
        if constexpr (std::is_signed_v<V>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(d, 0, "volume depth can not be less than zero");
        }
    }

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    VolumeSize(V w, V h, V d = 1) noexcept(std::is_unsigned_v<V>)
        : RectSize<D>(w, h)
        , m_depth(Data::RoundCast<D>(d))
    {
        if constexpr (std::is_signed_v<V>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(d, 0, "volume depth can not be less than zero");
        }
    }

    template<typename V, typename = std::enable_if_t<std::is_arithmetic_v<V>>>
    explicit VolumeSize(const Point3T<V>& point) noexcept(std::is_unsigned_v<V>)
        : RectSize<D>(point.GetX(), point.GetY())
        , m_depth(Data::RoundCast<D>(point.GetZ()))
    {
        if constexpr (std::is_signed_v<V>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(point.GetZ(), 0, "volume depth can not be less than zero");
        }
    }

    template<typename V, typename = std::enable_if_t<!std::is_same_v<D, V>>>
    explicit VolumeSize(const VolumeSize<V>& other) noexcept
        : RectSize<D>(other)
        , m_depth(Data::RoundCast<D>(other.GetDepth()))
    { }

    D GetDepth() const noexcept { return m_depth; }

    void SetDepth(D depth) noexcept(std::is_unsigned_v<D>)
    {
        if constexpr (std::is_signed_v<D>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(depth, 0, "volume depth can not be less than zero");
        }
        m_depth = depth;
    }

    bool operator==(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator==(other) && m_depth == other.m_depth; }

    bool operator!=(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator!=(other) || m_depth != other.m_depth; }

    bool operator<=(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator<=(other) && m_depth <= other.m_depth; }

    bool operator<(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator<(other) && m_depth < other.m_depth; }

    bool operator>=(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator>=(other) && m_depth >= other.m_depth; }

    bool operator>(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator>(other) && m_depth > other.m_depth; }

    VolumeSize operator+(const VolumeSize& other) const noexcept
    { return VolumeSize(RectSize<D>::operator+(other), m_depth + other.m_depth); }

    VolumeSize operator-(const VolumeSize& other) const noexcept
    { return VolumeSize(RectSize<D>::operator-(other), m_depth - other.m_depth); }

    VolumeSize& operator+=(const VolumeSize& other) noexcept
    { m_depth += other.m_depth; RectSize<D>::operator+=(other); return *this; }

    VolumeSize& operator-=(const VolumeSize& other) noexcept
    { m_depth -= other.m_depth; RectSize<D>::operator-=(other); return *this; }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize> operator*(M multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        D depth {};
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            depth = Data::RoundCast<D>(static_cast<M>(m_depth) * multiplier);
        else
            depth = m_depth * Data::RoundCast<D>(multiplier);
        return VolumeSize(RectSize<D>::operator*(multiplier), depth);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize> operator/(M divisor) const noexcept(std::is_unsigned_v<M>)
    {
        D depth {};
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            depth = Data::RoundCast<D>(static_cast<M>(m_depth) / divisor);
        else
            depth = m_depth / Data::RoundCast<D>(divisor);
        return VolumeSize(RectSize<D>::operator/(divisor), depth);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize&> operator*=(M multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            m_depth = Data::RoundCast<D>(static_cast<M>(m_depth) * multiplier);
        else
            m_depth *= Data::RoundCast<D>(multiplier);
        RectSize<D>::operator*=(multiplier);
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize&> operator/=(M divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            m_depth = Data::RoundCast<D>(static_cast<M>(m_depth) / divisor);
        else
            m_depth /= Data::RoundCast<D>(divisor);
        RectSize<D>::operator/=(divisor);
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize> operator*(const Point3T<M>& multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetZ(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        D depth {};
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            depth = Data::RoundCast<D>(static_cast<M>(m_depth) * multiplier.GetZ());
        else
            depth = m_depth * Data::RoundCast<D>(multiplier.GetZ());
        return VolumeSize(RectSize<D>::operator*(Point2T<M>(multiplier.GetX(), multiplier.GetY())), depth);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize> operator/(const Point3T<M>& divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetZ(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        D depth {};
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            depth = Data::RoundCast<D>(static_cast<M>(m_depth) / divisor.GetZ());
        else
            depth = m_depth / Data::RoundCast<D>(divisor.GetZ());
        return VolumeSize(RectSize<D>::operator/(Point2T<M>(divisor.GetX(), divisor.GetY())), depth);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize&> operator*=(const Point3T<M>& multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetZ(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            m_depth = Data::RoundCast<D>(static_cast<M>(m_depth) * multiplier.GetZ());
        else
            m_depth *= Data::RoundCast<D>(multiplier.GetZ());
        RectSize<D>::operator*=(Point2T<M>(multiplier.GetX(), multiplier.GetY()));
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize&> operator/=(const Point3T<M>& divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetZ(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            m_depth = Data::RoundCast<D>(static_cast<M>(m_depth) / divisor.GetZ());
        else
            m_depth /= Data::RoundCast<D>(divisor.GetZ());
        RectSize<D>::operator/=(Point2T<M>(divisor.GetX(), divisor.GetY()));
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize> operator*(const VolumeSize<M>& multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetDepth(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        D depth {};
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            depth = Data::RoundCast<D>(static_cast<M>(m_depth) * multiplier.GetDepth());
        else
            depth = m_depth * Data::RoundCast<D>(multiplier.GetDepth());
        return VolumeSize(RectSize<D>::operator*(multiplier), depth);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize> operator/(const VolumeSize<M>& divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetDepth(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        D depth {};
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            depth = Data::RoundCast<D>(static_cast<M>(m_depth) / divisor.GetDepth());
        else
            depth = m_depth / Data::RoundCast<D>(divisor.GetDepth());
        return VolumeSize(RectSize<D>::operator/(divisor), depth);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize&> operator*=(const VolumeSize<M>& multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier.GetDepth(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            m_depth = Data::RoundCast<D>(static_cast<M>(m_depth) * multiplier.GetDepth());
        else
            m_depth *= Data::RoundCast<D>(multiplier.GetDepth());
        RectSize<D>::operator*=(multiplier);
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, VolumeSize&> operator/=(const VolumeSize<M>& divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor.GetDepth(), 0, "volume size multiplier coordinate z can not be less than zero");
        }
        if constexpr (std::is_floating_point_v<M> && std::is_integral_v<D>)
            m_depth = Data::RoundCast<D>(static_cast<M>(m_depth) / divisor.GetDepth());
        else
            m_depth /= Data::RoundCast<D>(divisor.GetDepth());
        RectSize<D>::operator/=(divisor);
        return *this;
    }

    D GetPixelsCount() const noexcept { return m_depth * RectSize<D>::GetPixelsCount(); }
    D GetLongestSide() const noexcept { return std::max(m_depth, RectSize<D>::GetLongestSide()); }

    RectSize<D>& AsRectSize() noexcept             { return static_cast<RectSize<D>&>(*this); }
    const RectSize<D>& AsRectSize() const noexcept { return static_cast<const RectSize<D>&>(*this); }

    explicit operator bool() const noexcept { return RectSize<D>::operator bool() && m_depth; }

    template<typename V, typename = std::enable_if_t<!std::is_same_v<D, V>>>
    explicit operator VolumeSize<V>() const noexcept
    {
        return VolumeSize<V>(RectSize<D>::operator RectSize<V>(), Data::RoundCast<D>(m_depth));
    }

    explicit operator std::string() const noexcept
    {
        return fmt::format("Sz({} x {} x {})", RectSize<D>::GetWidth(), RectSize<D>::GetHeight(), m_depth);
    }

private:
    D m_depth = 1;
};

template<typename T, typename D>
struct Volume // NOSONAR - class has more than 35 methods
{
    using Point = Point3T<T>;
    using Size  = VolumeSize<D>;

    Point origin;
    Size  size;

    Volume() = default;
    explicit Volume(const Size& size) noexcept : size(size) { }
    explicit Volume(const Point& origin) noexcept : origin(origin) { }
    Volume(const Point& origin, const Size& size) noexcept : origin(origin), size(size) { }
    Volume(T x, T y, T z, D w, D h, D d) : origin(x, y, z), size(w, h, d) { }

    T GetLeft() const noexcept   { return origin.GetX(); }
    T GetRight() const noexcept  { return origin.GetX() + Data::RoundCast<T>(size.GetWidth()); }
    T GetTop() const noexcept    { return origin.GetY(); }
    T GetBottom() const noexcept { return origin.GetY() + Data::RoundCast<T>(size.GetHeight()); }
    T GetNear() const noexcept   { return origin.GetZ(); }
    T GetFar() const noexcept    { return origin.GetZ() + Data::RoundCast<T>(size.GetDepth()); }

    bool operator==(const Volume& other) const noexcept
    {
        return std::tie(origin, size) == std::tie(other.origin, other.size);
    }

    bool operator!=(const Volume& other) const noexcept
    {
        return std::tie(origin, size) != std::tie(other.origin, other.size);
    }

    bool operator<(const Volume& other) const noexcept
    {
        return std::tie(origin, size) < std::tie(other.origin, other.size);
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Volume<T, D>> operator*(M multiplier) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier, 0, "volume multiplier can not be less than zero");
        return Volume<T, D>{ origin * multiplier, size * multiplier };
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Volume<T, D>> operator/(M divisor) const noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor, 0, "volume divisor can not be less than zero");
        return Volume<T, D>{ origin / divisor, size / divisor };
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Volume<T, D>&> operator*=(M multiplier) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(multiplier, 0, "volume multiplier can not be less than zero");
        origin *= multiplier;
        size   *= multiplier;
        return *this;
    }

    template<typename M>
    std::enable_if_t<std::is_arithmetic_v<M>, Volume<T, D>&> operator/=(M divisor) noexcept(std::is_unsigned_v<M>)
    {
        if constexpr (std::is_signed_v<M>)
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(divisor, 0, "volume divisor can not be less than zero");
        origin /= divisor;
        size   /= divisor;
        return *this;
    }

    template<typename V, typename K, typename = std::enable_if_t<!std::is_same_v<T, V> || !std::is_same_v<D, K>>>
    explicit operator Volume<V, K>() const
    {
        return Volume<V, K>(static_cast<Point3T<V>>(origin), static_cast<VolumeSize<K>>(size));
    }

    explicit operator std::string() const
    {
        return fmt::format("Vol[{} : {}]", origin, size);
    }
};

using Dimensions = VolumeSize<uint32_t>;

using Viewport  = Volume<double, double>;
using Viewports = std::vector<Viewport>;

[[nodiscard]] Viewport GetFrameViewport(const FrameSize& frame_size);
[[nodiscard]] Viewport GetFrameViewport(const FrameRect& frame_rect);

} // namespace Methane::Graphics
