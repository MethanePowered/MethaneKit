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

#include <fmt/format.h>
#include <sstream>

namespace Methane::Graphics
{

template<typename D>
using RectSize = Data::RectSize<D>;

template<typename D>
struct VolumeSize : RectSize<D>
{
    D depth = 1;

    VolumeSize() = default;
    explicit VolumeSize(const RectSize<D>& rect_size, D d = 1) : RectSize<D>(rect_size), depth(d) { }
    VolumeSize(D w, D h, D d = 1) : RectSize<D>(w, h), depth(d) { }

    bool operator==(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator==(other) && depth == other.depth; }

    bool operator!=(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator!=(other) || depth != other.depth; }

    bool operator<=(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator<=(other) && depth <= other.depth; }

    bool operator<(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator<(other) && depth < other.depth; }

    bool operator>=(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator>=(other) && depth >= other.depth; }

    bool operator>(const VolumeSize& other) const noexcept
    { return RectSize<D>::operator>(other) && depth > other.depth; }

    VolumeSize operator+(const VolumeSize& other) const noexcept
    { return VolumeSize(RectSize<D>::operator+(other), depth + other.depth); }

    VolumeSize operator-(const VolumeSize& other) const noexcept
    { return VolumeSize(RectSize<D>::operator-(other), depth - other.depth); }

    template<typename M>
    VolumeSize operator*(M multiplier) const noexcept
    { return VolumeSize(RectSize<D>::operator*(multiplier), static_cast<D>(static_cast<M>(depth) * multiplier)); }

    template<typename M>
    VolumeSize operator/(M divisor) const noexcept
    { return VolumeSize(RectSize<D>::operator/(divisor), static_cast<D>(static_cast<M>(depth) / divisor)); }

    VolumeSize& operator+=(const VolumeSize& other) noexcept
    { depth += other.depth; return RectSize<D>::operator+=(other); }

    VolumeSize& operator-=(const VolumeSize& other) noexcept
    { depth -= other.depth; return RectSize<D>::operator-=(other); }

    template<typename M>
    VolumeSize& operator*=(M multiplier) noexcept
    {
        depth = static_cast<D>(static_cast<M>(depth) * multiplier);
        return RectSize<D>::operator*=(multiplier);
    }

    template<typename M>
    VolumeSize& operator/=(M divisor) noexcept
    {
        depth = static_cast<D>(static_cast<M>(depth) / divisor);
        return RectSize<D>::operator/=(divisor);
    }

    explicit operator bool() const noexcept
    { return depth && RectSize<D>::operator bool(); }

    D GetPixelsCount() const noexcept { return depth * RectSize<D>::GetPixelsCount(); }
    D GetLongestSide() const noexcept { return std::max(depth, RectSize<D>::GetLongestSide()); }

    explicit operator std::string() const
    {
        std::stringstream ss;
        ss << "Sz(" << std::to_string(RectSize<D>::width);
        if (RectSize<D>::height != 1 || depth != 1)
            ss << " x " << std::to_string(RectSize<D>::height);
        if (depth != 1)
            ss << " x " << std::to_string(depth);
        ss << ")";
        return ss.str();
    }
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

Viewport GetFrameViewport(const FrameSize& frame_size);
Viewport GetFrameViewport(const FrameRect& frame_rect);

} // namespace Methane::Graphics
