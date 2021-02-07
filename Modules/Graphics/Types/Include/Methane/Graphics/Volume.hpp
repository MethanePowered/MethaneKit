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
class VolumeSize : public RectSize<D>
{
public:
    VolumeSize() = default;

    explicit VolumeSize(const RectSize<D>& rect_size, D d = 1) noexcept(std::is_unsigned_v<D>)
        : RectSize<D>(rect_size)
            , m_depth(d)
    {
        if constexpr (std::is_signed_v<D>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(d, 0, "volume depth can not be less than zero");
        }
    }

    VolumeSize(D w, D h, D d = 1) noexcept(std::is_unsigned_v<D>)
        : RectSize<D>(w, h)
        , m_depth(d)
    {
        if constexpr (std::is_signed_v<D>)
        {
            META_CHECK_ARG_GREATER_OR_EQUAL_DESCR(d, 0, "volume depth can not be less than zero");
        }
    }

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
    { m_depth += other.m_depth; return RectSize<D>::operator+=(other); }

    VolumeSize& operator-=(const VolumeSize& other) noexcept
    { m_depth -= other.m_depth; return RectSize<D>::operator-=(other); }

    template<typename M>
    VolumeSize operator*(M multiplier) const noexcept(std::is_unsigned_v<M>)
    { return VolumeSize(RectSize<D>::operator*(multiplier), static_cast<D>(static_cast<M>(m_depth) * multiplier)); }

    template<typename M>
    VolumeSize operator/(M divisor) const noexcept(std::is_unsigned_v<M>)
    { return VolumeSize(RectSize<D>::operator/(divisor), static_cast<D>(static_cast<M>(m_depth) / divisor)); }

    template<typename M>
    VolumeSize& operator*=(M multiplier) noexcept(std::is_unsigned_v<M>)
    {
        m_depth = static_cast<D>(static_cast<M>(m_depth) * multiplier);
        return RectSize<D>::operator*=(multiplier);
    }

    template<typename M>
    VolumeSize& operator/=(M divisor) noexcept(std::is_unsigned_v<M>)
    {
        m_depth = static_cast<D>(static_cast<M>(m_depth) / divisor);
        return RectSize<D>::operator/=(divisor);
    }

    explicit operator bool() const noexcept
    { return m_depth && RectSize<D>::operator bool(); }

    D GetPixelsCount() const noexcept { return m_depth * RectSize<D>::GetPixelsCount(); }
    D GetLongestSide() const noexcept { return std::max(m_depth, RectSize<D>::GetLongestSide()); }

    explicit operator std::string() const noexcept
    {
        return fmt::format("Sz({} x {} x {})", RectSize<D>::GetWidth(), RectSize<D>::GetHeight(), m_depth);
    }

private:
    D m_depth = 1;
};

template<typename T, typename D>
struct Volume
{
    using Point = Point3T<T>;
    using Size  = VolumeSize<D>;

    Point origin;
    Size  size;

    bool operator==(const Volume& other) const noexcept
    { return std::tie(origin, size) == std::tie(other.origin, other.size); }

    bool operator!=(const Volume& other) const noexcept
    { return std::tie(origin, size) != std::tie(other.origin, other.size); }

    template<typename M>
    Volume<T, D> operator*(M multiplier) const noexcept
    { return Volume<T, D>{ origin * multiplier, size * multiplier }; }

    template<typename M>
    Volume<T, D> operator/(M divisor) const noexcept
    { return Volume<T, D>{ origin / divisor, size / divisor }; }

    template<typename M>
    Volume<T, D>& operator*=(M multiplier) noexcept
    { origin *= multiplier; size *= multiplier; return *this; }

    template<typename M>
    Volume<T, D>& operator/=(M divisor) noexcept
    { origin /= divisor; size /= divisor; return *this; }

    explicit operator std::string() const
    { return fmt::format("Vm[{}, {}]", origin, size); }
};

using Dimensions = VolumeSize<uint32_t>;

using Viewport  = Volume<double, double>;
using Viewports = std::vector<Viewport>;

[[nodiscard]] Viewport GetFrameViewport(const FrameSize& frame_size);
[[nodiscard]] Viewport GetFrameViewport(const FrameRect& frame_rect);

} // namespace Methane::Graphics
