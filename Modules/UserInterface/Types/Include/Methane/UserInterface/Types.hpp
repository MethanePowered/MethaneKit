/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/Types.h
Methane user interface types root header.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Color.hpp>

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

enum class Units : uint8_t
{
    Pixels = 0u,
    Dots,
};

inline std::string UnitsToString(Units units) noexcept
{
    switch(units)
    {
    case Units::Pixels: return "pixels";
    case Units::Dots:   return "dots";
    }
}

struct UnitSize : FrameSize
{
    Units units = Units::Pixels;

    using FrameSize::FrameSize;

    explicit UnitSize(const FrameSize& size) noexcept : FrameSize(size), units(Units::Pixels) { }
    UnitSize(const FrameSize& size, Units units) noexcept : FrameSize(size), units(units) { }
    UnitSize(DimensionType w, DimensionType h, Units units) noexcept : FrameSize(w, h), units(units) { }

    inline void CheckUnitsCompatibility(const UnitSize& other) const { if (units != other.units) throw std::invalid_argument("Incompatible size units."); }

    using FrameSize::operator bool;
    operator std::string() const                                         { return FrameSize::operator std::string() + " in " + UnitsToString(units); }

    bool operator==(const UnitSize& other) const noexcept                { return FrameSize::operator==(other) && units == other.units; }
    bool operator!=(const UnitSize& other) const noexcept                { return FrameSize::operator!=(other) || units != other.units; }
    bool operator<=(const UnitSize& other) const noexcept                { return FrameSize::operator<=(other) && units == other.units; }
    bool operator<(const UnitSize& other) const noexcept                 { return FrameSize::operator<(other)  && units == other.units; }
    bool operator>=(const UnitSize& other) const noexcept                { return FrameSize::operator>=(other) && units == other.units; }
    bool operator>(const UnitSize& other) const noexcept                 { return FrameSize::operator>(other)  && units == other.units; }

    UnitSize operator+(const UnitSize& other) const                      { CheckUnitsCompatibility(other); return UnitSize(FrameSize::operator+(other), units); }
    UnitSize operator-(const UnitSize& other) const                      { CheckUnitsCompatibility(other); return UnitSize(FrameSize::operator-(other), units); }
    UnitSize& operator+=(const UnitSize& other) noexcept                 { CheckUnitsCompatibility(other); FrameSize::operator+=(other); return *this; }
    UnitSize& operator-=(const UnitSize& other) noexcept                 { CheckUnitsCompatibility(other); FrameSize::operator-=(other); return *this; }

    template<typename M> UnitSize operator*(M multiplier) const noexcept                    { return UnitSize(FrameSize::operator*(multiplier), units); }
    template<typename M> UnitSize operator/(M divisor) const noexcept                       { return UnitSize(FrameSize::operator/(divisor), units); }
    template<typename M> UnitSize& operator*=(M multiplier) noexcept                        { FrameSize::operator*=(multiplier); return *this; }
    template<typename M> UnitSize& operator/=(M divisor) noexcept                           { FrameSize::operator/=(divisor); return *this; }

    template<typename M> using Point = Data::Point2T<M>;
    template<typename M> UnitSize operator*(const Point<M>& multiplier) const noexcept      { return UnitSize(FrameSize::operator*(multiplier), units); }
    template<typename M> UnitSize operator/(const Point<M>& divisor) const noexcept         { return UnitSize(FrameSize::operator/(divisor), units); }
    template<typename M> UnitSize& operator*=(const Point<M>& multiplier) noexcept          { FrameSize::operator*=(multiplier); return *this; }
    template<typename M> UnitSize& operator/=(const Point<M>& divisor) noexcept             { FrameSize::operator/=(divisor); return *this; }

    template<typename M> using RectSize = Data::RectSize<M>;
    template<typename M> UnitSize operator*(const RectSize<M>& multiplier) const noexcept   { return UnitSize(FrameSize::operator*(multiplier), units); }
    template<typename M> UnitSize operator/(const RectSize<M>& divisor) const noexcept      { return UnitSize(FrameSize::operator/(divisor), units); }
    template<typename M> UnitSize& operator*=(const RectSize<M>& multiplier) noexcept       { FrameSize::operator*=(multiplier); return *this; }
    template<typename M> UnitSize& operator/=(const RectSize<M>& divisor) noexcept          { FrameSize::operator/=(divisor); return *this; }
};

struct UnitPoint : FramePoint
{
    Units units = Units::Pixels;

    using FramePoint::FramePoint;

    UnitPoint(const FramePoint& point, Units units) noexcept : FramePoint(point), units(units) { }
    UnitPoint(CoordinateType x, CoordinateType y, Units units) noexcept : FramePoint(x, y), units(units) { }
    UnitPoint(const UnitSize& size) noexcept : UnitPoint(size.width, size.height, size.units) { }

    inline void CheckUnitsCompatibility(const UnitPoint& other) const { if (units != other.units) throw std::invalid_argument("Incompatible point units."); }

    operator std::string() const                                                            { return FramePoint::operator std::string() + " in " + UnitsToString(units); }

    bool operator==(const UnitPoint& other) const noexcept                                  { return static_cast<const FramePoint&>(*this) == other && units == other.units; }
    bool operator!=(const UnitPoint& other) const noexcept                                  { return static_cast<const FramePoint&>(*this) != other || units != other.units; }
    bool operator<=(const UnitPoint& other) const noexcept                                  { return static_cast<const FramePoint&>(*this) <= other && units == other.units; }
    bool operator<(const UnitPoint& other) const noexcept                                   { return static_cast<const FramePoint&>(*this) <  other && units == other.units; }
    bool operator>=(const UnitPoint& other) const noexcept                                  { return static_cast<const FramePoint&>(*this) >= other && units == other.units; }
    bool operator>(const UnitPoint& other) const noexcept                                   { return static_cast<const FramePoint&>(*this) >  other && units == other.units; }

    UnitPoint operator+(const UnitPoint& other) const                                       { CheckUnitsCompatibility(other); return UnitPoint(static_cast<const FramePoint&>(*this) + other, units); }
    UnitPoint operator-(const UnitPoint& other) const                                       { CheckUnitsCompatibility(other); return UnitPoint(static_cast<const FramePoint&>(*this) - other, units); }
    UnitPoint& operator+=(const UnitPoint& other) noexcept                                  { CheckUnitsCompatibility(other); FramePoint::operator+=(other); return *this; }
    UnitPoint& operator-=(const UnitPoint& other) noexcept                                  { CheckUnitsCompatibility(other); FramePoint::operator-=(other); return *this; }

    template<typename M> UnitPoint  operator*(M multiplier) const noexcept                  { return UnitPoint(FramePoint::operator*(multiplier), units); }
    template<typename M> UnitPoint  operator/(M divisor) const noexcept                     { return UnitPoint(FramePoint::operator/(divisor), units); }
    template<typename M> UnitPoint& operator*=(M multiplier) noexcept                       { FramePoint::operator*=(multiplier); return *this; }
    template<typename M> UnitPoint& operator/=(M divisor) noexcept                          { FramePoint::operator/=(divisor); return *this; }

    template<typename M> UnitPoint  operator*(const Point2T<M>& multiplier) const noexcept  { return UnitPoint(FramePoint::operator*(multiplier), units); }
    template<typename M> UnitPoint  operator/(const Point2T<M>& divisor) const noexcept     { return UnitPoint(FramePoint::operator/(divisor), units); }
    template<typename M> UnitPoint& operator*=(const Point2T<M>& multiplier) noexcept       { FramePoint::operator*=(multiplier); return *this; }
    template<typename M> UnitPoint& operator/=(const Point2T<M>& divisor) noexcept          { FramePoint::operator/=(divisor); return *this; }
};

struct UnitRect : FrameRect
{
    Units units = Units::Pixels;

    using FrameRect::FrameRect;

    UnitRect(const FrameRect& rect, Units units) noexcept   : FrameRect(rect), units(units) { }
    UnitRect(Point origin, Size size, Units units) noexcept : FrameRect(origin, size), units(units) { }

    operator std::string() const                                            { return FrameRect::operator std::string() + " in " + UnitsToString(units); }
    bool operator==(const UnitRect& other) const noexcept                   { return FrameRect::operator==(other) && units == other.units; }
    bool operator!=(const UnitRect& other) const noexcept                   { return FrameRect::operator!=(other) || units != other.units; }

    template<typename M> UnitRect operator*(M multiplier) const noexcept    { return UnitRect(FrameRect::operator*(multiplier), units); }
    template<typename M> UnitRect operator/(M divisor) const noexcept       { return UnitRect(FrameRect::operator/(divisor), units); }
    template<typename M> UnitRect& operator*=(M multiplier) noexcept        { FrameRect::operator*=(multiplier); return *this; }
    template<typename M> UnitRect& operator/=(M divisor) noexcept           { FrameRect::operator/=(divisor); return *this; }
};

} // namespace Methane::UserInterface
