/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Tests/UserInterface/Types/UnitTypeHelpers.hpp
Unit-test helpers for User Interface Unit types

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Types.hpp>

#include <catch2/catch.hpp>

#define POINT_BASE_TYPES Methane::Data::FramePoint, Methane::Data::FloatPoint
#define SIZE_BASE_TYPES  Methane::Data::FrameSize,  Methane::Data::FloatSize
#define RECT_BASE_TYPES  Methane::Data::FrameRect,  Methane::Data::FloatRect

#define ALL_BASE_TYPES POINT_BASE_TYPES, SIZE_BASE_TYPES, RECT_BASE_TYPES

namespace Catch
{
    template<typename BaseType>
    struct StringMaker<Methane::UserInterface::UnitType<BaseType>>
    {
        static std::string convert(const Methane::UserInterface::UnitType<BaseType>& value)
        {
            return static_cast<std::string>(value);
        }
    };
}

namespace Methane::UserInterface
{

template<typename T>
void CheckUnitType(const UnitType<Data::Point2T<T>>& unit_point, const Data::Point2T<T>& orig_point, Units units)
{
    CHECK(unit_point.GetUnits() == units);
    CHECK(unit_point.GetX() == orig_point.GetX());
    CHECK(unit_point.GetY() == orig_point.GetY());
}

template<typename T>
void CheckUnitType(const UnitType<Data::RectSize<T>>& unit_size, const Data::RectSize<T>& orig_size, Units units)
{
    CHECK(unit_size.GetUnits()  == units);
    CHECK(unit_size.GetWidth()  == orig_size.GetWidth());
    CHECK(unit_size.GetHeight() == orig_size.GetHeight());
}

template<typename T, typename D>
void CheckUnitType(const UnitType<Data::Rect<T, D>>& unit_rect, const Data::Rect<T, D>& orig_rect, Units units)
{
    CHECK(unit_rect.GetUnits()  == units);
    CHECK(unit_rect.origin.GetX() == orig_rect.origin.GetX());
    CHECK(unit_rect.origin.GetY() == orig_rect.origin.GetY());
    CHECK(unit_rect.size.GetWidth()  == orig_rect.size.GetWidth());
    CHECK(unit_rect.size.GetHeight() == orig_rect.size.GetHeight());
}

template<typename T, typename S>
T GetMultipliedValue(T value, S mult)
{
    if constexpr (std::is_floating_point_v<S>)
        return Data::RoundCast<T>(static_cast<S>(value) * mult);
    else
        return value * static_cast<T>(mult);
}

template<typename T, typename S>
void CreateTestItem(Data::Point2T<T>& test_item, S mult)
{
    test_item = Data::Point2T<T>(GetMultipliedValue(T(12), mult), GetMultipliedValue(T(23), mult));
}

template<typename T, typename S>
void CreateTestItem(Data::RectSize<T>& test_item, S mult)
{
    test_item = Methane::Data::RectSize<T>(GetMultipliedValue(T(123), mult), GetMultipliedValue(T(234), mult));
}

template<typename T, typename D, typename S>
void CreateTestItem(Data::Rect<T, D>& test_item, S mult)
{
    test_item = Methane::Data::Rect<T, D>(GetMultipliedValue(T(12), mult), GetMultipliedValue(T(23), mult), GetMultipliedValue(D(123), mult), GetMultipliedValue(D(234), mult));
}

template<typename TestType, typename S = int32_t>
TestType CreateTestItem(S mult = S(1))
{
    TestType test_item; CreateTestItem(test_item, mult); return test_item;
}

template<typename T, typename S>
void CreateUnitItem(Units units, UnitType<Data::Point2T<T>>& test_item, S mult)
{
    test_item = UnitType<Methane::Data::Point2T<T>>(units, GetMultipliedValue(T(12), mult), GetMultipliedValue(T(23), mult));
}

template<typename T, typename S>
void CreateUnitItem(Units units, UnitType<Data::RectSize<T>>& test_item, S mult)
{
    test_item = UnitType<Methane::Data::RectSize<T>>(units, GetMultipliedValue(T(123), mult), GetMultipliedValue(T(234), mult));
}

template<typename T, typename D, typename S>
void CreateUnitItem(Units units, UnitType<Data::Rect<T, D>>& test_item, S mult)
{
    test_item = UnitType<Methane::Data::Rect<T, D>>(units, GetMultipliedValue(T(12), mult), GetMultipliedValue(T(23), mult), GetMultipliedValue(D(123), mult), GetMultipliedValue(D(234), mult));
}

template<typename TestType, typename S = int32_t>
UnitType<TestType> CreateUnitItem(Units units, S mult = S(1))
{
    UnitType<TestType> test_item; CreateUnitItem(units, test_item, mult); return test_item;
}

} // namespace Methane::UserInterface
