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

namespace gfx = Methane::Graphics;

namespace Methane::UserInterface
{

using FrameRect    = Graphics::FrameRect;
using FrameSize    = Graphics::FrameSize;
using FramePoint   = Graphics::FramePoint;

using FloatRect    = Graphics::FloatRect;
using FloatSize    = Graphics::FloatSize;
using FloatPoint   = Graphics::FloatPoint;

using Color3F      = Graphics::Color3F;
using Color4F      = Graphics::Color4F;

template<typename T>
using Point2T      = Data::Point<T, 2>;

enum class Units : uint8_t
{
    Pixels,
    Dots,
};

constexpr std::string_view GetUnitsName(Units units) noexcept
{
    switch(units)
    {
    case Units::Pixels: return "pixels";
    case Units::Dots:   return "dots";
    default:            return "???";
    }
}

template<typename BaseType>
class UnitType : public BaseType
{
    template<typename T, typename V, typename R>
    using EnableReturnTypeIf = std::enable_if_t<std::is_same_v<V, T>, R>;

    template<typename T, typename R>
    using EnableReturnType = EnableReturnTypeIf<T, std::decay_t<R>, R>;

public:
    using Base = BaseType;

    using BaseType::BaseType;
    explicit UnitType(const BaseType& base) noexcept     : BaseType(base) { }
    UnitType(Units units, const BaseType& base) noexcept : BaseType(base), m_units(units) { }
    UnitType(Units units, BaseType&& base) noexcept      : BaseType(std::move(base)), m_units(units) { }

    template<typename V>
    explicit UnitType(const UnitType<V>& other) noexcept : BaseType(static_cast<const V&>(other)), m_units(other.GetUnits()) { }

    // Disable Sonar Check for variadic arguments constructor, since it reports false positive about slicing for universal references
    template<typename... BaseArgs>
    explicit UnitType(Units units, BaseArgs&&... base_args) noexcept
        : BaseType(std::forward<BaseArgs>(base_args)...)
        , m_units(units)
    { }

    template<typename T = BaseType, typename = std::enable_if_t<std::is_same_v<FramePoint, T>>>
    explicit UnitType(const UnitType<FrameSize>& size) noexcept
        : UnitType<FramePoint>(size.GetUnits(), size.GetWidth(), size.GetHeight())
    { }

    Units GetUnits() const noexcept { return m_units; }

    [[nodiscard]] friend auto operator<=>(const UnitType& left, const UnitType& right)
    {
        META_CHECK_EQUAL(right.GetUnits(), left.m_units);
        return static_cast<const BaseType&>(left) <=> static_cast<const BaseType&>(right);
    }

    friend bool operator==(const UnitType& left, const UnitType& right)
    {
        return std::is_eq(left <=> right);
    }

    friend UnitType operator+(const UnitType& left, const UnitType& right)
    {
        META_CHECK_EQUAL(right.GetUnits(), left.m_units);
        return UnitType<BaseType>(left.m_units, static_cast<const BaseType&>(left) + right);
    }

    friend UnitType operator-(const UnitType& left, const UnitType& right)
    {
        META_CHECK_EQUAL(right.GetUnits(), left.m_units);
        return UnitType<BaseType>(left.m_units, static_cast<const BaseType&>(left) - right);
    }

    template<typename T> friend UnitType operator*(const UnitType& p, T&& multiplier) noexcept
    {
        return UnitType<BaseType>(p.m_units, static_cast<const BaseType&>(p) * std::forward<T>(multiplier));
    }

    template<typename T> friend UnitType operator/(const UnitType& p, T&& divisor) noexcept
    {
        return UnitType<BaseType>(p.m_units, static_cast<const BaseType&>(p) / std::forward<T>(divisor));
    }

    UnitType& operator+=(const UnitType& other)
    {
        META_CHECK_EQUAL(other.GetUnits(), m_units);
        BaseType::operator+=(other);
        return *this;
    }

    UnitType& operator-=(const UnitType& other)
    {
        META_CHECK_EQUAL(other.GetUnits(), m_units);
        BaseType::operator-=(other);
        return *this;
    }

    template<typename T> UnitType& operator*=(T&& multiplier) noexcept
    {
        BaseType::operator*=(std::forward<T>(multiplier));
        return *this;
    }

    template<typename T> UnitType& operator/=(T&& divisor) noexcept
    {
        BaseType::operator/=(std::forward<T>(divisor));
        return *this;
    }

    BaseType&       AsBase() noexcept         { return static_cast<BaseType&>(*this); }
    const BaseType& AsBase() const noexcept   { return static_cast<const BaseType&>(*this); }

    template<typename T = BaseType, typename C = typename T::CoordinateType, typename D = typename T::DimensionType>
    EnableReturnTypeIf<T, Data::Rect<C, D>, UnitType<Data::Point2T<C>>> GetUnitOrigin() const noexcept
    {
        return UnitType<Data::Point2T<C>>(m_units, BaseType::origin);
    }

    template<typename T = BaseType, typename C = typename T::CoordinateType, typename D = typename T::DimensionType>
    EnableReturnTypeIf<T, Data::Rect<C, D>, UnitType<Data::RectSize<D>>>  GetUnitSize() const noexcept
    {
        return UnitType<Data::RectSize<D>>(m_units, BaseType::size);
    }

    explicit operator std::string() const
    {
        return fmt::format("{:s} in {:s}", BaseType::operator std::string(), GetUnitsName(m_units));
    }

private:
    Units m_units = Units::Pixels;
};

using UnitSize  = UnitType<FrameSize>;
using UnitPoint = UnitType<FramePoint>;
using UnitRect  = UnitType<FrameRect>;

} // namespace Methane::UserInterface
