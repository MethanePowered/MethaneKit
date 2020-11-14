/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Types.h
Methane user interface types root header.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Point.hpp>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

namespace Methane::UserInterface
{

using FrameRect    = Graphics::FrameRect;
using FrameSize    = Graphics::FrameSize;
using FramePoint   = Graphics::FramePoint;

using FloatRect    = Graphics::FloatRect;
using FloatSize    = Graphics::FloatSize;
using FloatPoint   = Graphics::FloatPoint;

using Color3f      = Graphics::Color3f;
using Color4f      = Graphics::Color4f;

template<typename T>
using Point2T      = Data::PointT<T, 2>;

enum class Units : uint8_t
{
    Pixels = 0U,
    Dots,
};

inline std::string UnitsToString(Units units) noexcept
{
    switch(units)
    {
    case Units::Pixels: return "pixels";
    case Units::Dots:   return "dots";
    default:            return "";
    }
}

template<typename BaseType>
struct UnitType : BaseType
{
    Units units = Units::Pixels;

    using BaseType::BaseType;

    explicit UnitType(const BaseType& base) noexcept     : BaseType(base), units(Units::Pixels) { }
    UnitType(Units units, const BaseType& base) noexcept : BaseType(base), units(units) { }
    UnitType(Units units, BaseType&& base) noexcept      : BaseType(std::move(base)), units(units) { }

    template<typename... BaseArgs>
    UnitType(Units units, BaseArgs&&... base_args) noexcept : BaseType(std::forward<BaseArgs>(base_args)...), units(units) { }

    explicit operator std::string() const                  { return fmt::format("{:s} in {:s}", BaseType::operator std::string(), UnitsToString(units)); }
    bool operator==(const UnitType& other) const noexcept  { return BaseType::operator==(other) && units == other.units; }
    bool operator!=(const UnitType& other) const noexcept  { return BaseType::operator!=(other) || units != other.units; }
};

struct UnitSize : UnitType<FrameSize>
{
    using UnitType<FrameSize>::UnitType;
    UnitSize(Units units, DimensionType w, DimensionType h) noexcept : UnitType<FrameSize>(units, w, h) { }

    FrameSize&       AsSize() noexcept       { return static_cast<FrameSize&>(*this); }
    const FrameSize& AsSize() const noexcept { return static_cast<const FrameSize&>(*this); }

    using FrameSize::operator bool;
    using UnitType<FrameSize>::operator==;
    using UnitType<FrameSize>::operator!=;

    bool operator<=(const UnitSize& other) const noexcept                  { return units == other.units && FrameSize::operator<=(other); }
    bool operator<(const UnitSize& other) const noexcept                   { return units == other.units && FrameSize::operator<(other); }
    bool operator>=(const UnitSize& other) const noexcept                  { return units == other.units && FrameSize::operator>=(other); }
    bool operator>(const UnitSize& other) const noexcept                   { return units == other.units && FrameSize::operator>(other); }

    UnitSize operator+(const UnitSize& other) const                        { META_CHECK_ARG_EQUAL(other.units, units); return UnitSize(units, FrameSize::operator+(other)); }
    UnitSize operator-(const UnitSize& other) const                        { META_CHECK_ARG_EQUAL(other.units, units); return UnitSize(units, FrameSize::operator-(other)); }
    UnitSize& operator+=(const UnitSize& other)                            { META_CHECK_ARG_EQUAL(other.units, units); FrameSize::operator+=(other); return *this; }
    UnitSize& operator-=(const UnitSize& other)                            { META_CHECK_ARG_EQUAL(other.units, units); FrameSize::operator-=(other); return *this; }

    template<typename M> UnitSize operator*(M multiplier) const noexcept                    { return UnitSize(units, FrameSize::operator*(multiplier)); }
    template<typename M> UnitSize operator/(M divisor) const noexcept                       { return UnitSize(units, FrameSize::operator/(divisor)); }
    template<typename M> UnitSize& operator*=(M multiplier) noexcept                        { FrameSize::operator*=(multiplier); return *this; }
    template<typename M> UnitSize& operator/=(M divisor) noexcept                           { FrameSize::operator/=(divisor); return *this; }

    template<typename M> using Point = Data::Point2T<M>;
    template<typename M> UnitSize operator*(const Point<M>& multiplier) const noexcept      { return UnitSize(units, FrameSize::operator*(multiplier)); }
    template<typename M> UnitSize operator/(const Point<M>& divisor) const noexcept         { return UnitSize(units, FrameSize::operator/(divisor)); }
    template<typename M> UnitSize& operator*=(const Point<M>& multiplier) noexcept          { FrameSize::operator*=(multiplier); return *this; }
    template<typename M> UnitSize& operator/=(const Point<M>& divisor) noexcept             { FrameSize::operator/=(divisor); return *this; }

    template<typename M> using RectSize = Data::RectSize<M>;
    template<typename M> UnitSize operator*(const RectSize<M>& multiplier) const noexcept   { return UnitSize(units, FrameSize::operator*(multiplier)); }
    template<typename M> UnitSize operator/(const RectSize<M>& divisor) const noexcept      { return UnitSize(units, FrameSize::operator/(divisor)); }
    template<typename M> UnitSize& operator*=(const RectSize<M>& multiplier) noexcept       { FrameSize::operator*=(multiplier); return *this; }
    template<typename M> UnitSize& operator/=(const RectSize<M>& divisor) noexcept          { FrameSize::operator/=(divisor); return *this; }
};

struct UnitPoint : UnitType<FramePoint>
{
    using UnitType<FramePoint>::UnitType;
    UnitPoint(Units units, CoordinateType x, CoordinateType y) noexcept : UnitType<FramePoint>(units, x, y) { }
    explicit UnitPoint(const UnitSize& size) noexcept : UnitPoint(size.units, size.width, size.height) { }

    FramePoint&       AsPoint() noexcept       { return static_cast<FramePoint&>(*this); }
    const FramePoint& AsPoint() const noexcept { return static_cast<const FramePoint&>(*this); }

    using UnitType<FramePoint>::operator==;
    using UnitType<FramePoint>::operator!=;

    bool operator<=(const UnitPoint& other) const noexcept                 { return units == other.units && FramePoint::operator<=(other); }
    bool operator<(const UnitPoint& other) const noexcept                  { return units == other.units && FramePoint::operator<(other);  }
    bool operator>=(const UnitPoint& other) const noexcept                 { return units == other.units && FramePoint::operator>=(other); }
    bool operator>(const UnitPoint& other) const noexcept                  { return units == other.units && FramePoint::operator>(other);  }

    UnitPoint operator+(const UnitPoint& other) const                      { META_CHECK_ARG_EQUAL(other.units, units); return UnitPoint(units, static_cast<const FramePoint&>(*this) + other); }
    UnitPoint operator-(const UnitPoint& other) const                      { META_CHECK_ARG_EQUAL(other.units, units); return UnitPoint(units, static_cast<const FramePoint&>(*this) - other); }
    UnitPoint& operator+=(const UnitPoint& other)                          { META_CHECK_ARG_EQUAL(other.units, units); FramePoint::operator+=(other); return *this; }
    UnitPoint& operator-=(const UnitPoint& other)                          { META_CHECK_ARG_EQUAL(other.units, units); FramePoint::operator-=(other); return *this; }

    template<typename M> UnitPoint  operator*(M multiplier) const noexcept                  { return UnitPoint(units, FramePoint::operator*(multiplier)); }
    template<typename M> UnitPoint  operator/(M divisor) const noexcept                     { return UnitPoint(units, FramePoint::operator/(divisor)); }
    template<typename M> UnitPoint& operator*=(M multiplier) noexcept                       { FramePoint::operator*=(multiplier); return *this; }
    template<typename M> UnitPoint& operator/=(M divisor) noexcept                          { FramePoint::operator/=(divisor); return *this; }

    template<typename M> UnitPoint  operator*(const Point2T<M>& multiplier) const noexcept  { return UnitPoint(units, FramePoint::operator*(multiplier)); }
    template<typename M> UnitPoint  operator/(const Point2T<M>& divisor) const noexcept     { return UnitPoint(units, FramePoint::operator/(divisor)); }
    template<typename M> UnitPoint& operator*=(const Point2T<M>& multiplier) noexcept       { FramePoint::operator*=(multiplier); return *this; }
    template<typename M> UnitPoint& operator/=(const Point2T<M>& divisor) noexcept          { FramePoint::operator/=(divisor); return *this; }
};

struct UnitRect : UnitType<FrameRect>
{
    using UnitType<FrameRect>::UnitType;
    explicit UnitRect(const UnitPoint& origin, const Size& size = {}) noexcept : UnitType<FrameRect>(origin.units, origin.AsPoint(), size) { }
    UnitRect(Units units, const Point& origin, const Size& size) noexcept : UnitType<FrameRect>(units, origin, size) { }

    FrameRect&       AsRect() noexcept          { return static_cast<FrameRect&>(*this); }
    const FrameRect& AsRect() const noexcept    { return static_cast<const FrameRect&>(*this); }

    UnitPoint GetUnitOrigin() const noexcept    { return UnitPoint(units, origin); }
    UnitSize  GetUnitSize() const noexcept      { return UnitSize(units, size); }

    using UnitType<FrameRect>::operator==;
    using UnitType<FrameRect>::operator!=;

    template<typename M> UnitRect operator*(M multiplier) const noexcept    { return UnitRect(units, FrameRect::operator*(multiplier)); }
    template<typename M> UnitRect operator/(M divisor) const noexcept       { return UnitRect(units, FrameRect::operator/(divisor)); }
    template<typename M> UnitRect& operator*=(M multiplier) noexcept        { FrameRect::operator*=(multiplier); return *this; }
    template<typename M> UnitRect& operator/=(M divisor) noexcept           { FrameRect::operator/=(divisor); return *this; }
};

} // namespace Methane::UserInterface
