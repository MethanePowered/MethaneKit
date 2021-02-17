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

FILE: Tests/UserInterface/Types/UnitTypesTest.cpp
Unit-tests of the Unit Types

******************************************************************************/

#include <Methane/UserInterface/Types.hpp>

#include <catch2/catch.hpp>

using namespace Methane;
using namespace Methane::UserInterface;

#define POINT_BASE_TYPES Data::FramePoint, Data::FloatPoint
#define SIZE_BASE_TYPES  Data::FrameSize,  Data::FloatSize
#define RECT_BASE_TYPES  Data::FrameRect,  Data::FloatRect

#define ALL_BASE_TYPES POINT_BASE_TYPES, SIZE_BASE_TYPES, RECT_BASE_TYPES

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
T GetShiftedValue(T value, S shift) { return value + static_cast<T>(shift); }

template<typename T, typename S>
void CreateTestItem(Data::Point2T<T>& test_item, S shift)
{ test_item = Data::Point2T<T>(GetShiftedValue(T(12), shift), GetShiftedValue(T(23), shift)); }

template<typename T, typename S>
void CreateTestItem(Data::RectSize<T>& test_item, S shift)
{ test_item = Methane::Data::RectSize<T>(GetShiftedValue(T(123), shift), GetShiftedValue(T(234), shift)); }

template<typename T, typename D, typename S>
void CreateTestItem(Data::Rect<T, D>& test_item, S shift)
{ test_item = Methane::Data::Rect<T, D>(GetShiftedValue(T(12), shift), GetShiftedValue(T(23), shift), GetShiftedValue(D(123), shift), GetShiftedValue(D(234), shift)); }

template<typename TestType, typename S = int32_t>
TestType CreateTestItem(S shift = 0)
{ TestType test_item; CreateTestItem(test_item, shift); return test_item; }

template<typename T, typename S>
void CreateUnitItem(Units units, UnitType<Data::Point2T<T>>& test_item, S shift)
{ test_item = UnitType<Methane::Data::Point2T<T>>(units, GetShiftedValue(T(12), shift), GetShiftedValue(T(23), shift)); }

template<typename T, typename S>
void CreateUnitItem(Units units, UnitType<Data::RectSize<T>>& test_item, S shift)
{ test_item = UnitType<Methane::Data::RectSize<T>>(units, GetShiftedValue(T(123), shift), GetShiftedValue(T(234), shift)); }

template<typename T, typename D, typename S>
void CreateUnitItem(Units units, UnitType<Data::Rect<T, D>>& test_item, S shift)
{ test_item = UnitType<Methane::Data::Rect<T, D>>(units, GetShiftedValue(T(12), shift), GetShiftedValue(T(23), shift), GetShiftedValue(D(123), shift), GetShiftedValue(D(234), shift)); }

template<typename TestType, typename S = int32_t>
UnitType<TestType> CreateUnitItem(Units units, S shift = 0)
{ UnitType<TestType> test_item; CreateUnitItem(units, test_item, shift); return test_item; }

TEMPLATE_TEST_CASE("Unit Type Initialization", "[unit][type][init]", ALL_BASE_TYPES)
{
    SECTION("Default constructor initialization")
    {
        CheckUnitType(UnitType<TestType>(), TestType(), Units::Pixels);
    }

    SECTION("Initialize with a original type reference")
    {
        const TestType test_item = CreateTestItem<TestType>();
        CheckUnitType(UnitType<TestType>(test_item), test_item, Units::Pixels);
    }

    SECTION("Initialize with units and original type reference")
    {
        const TestType test_item = CreateTestItem<TestType>();
        CheckUnitType(UnitType<TestType>(Units::Dots, test_item), test_item, Units::Dots);
    }

    SECTION("Initialize with units and original type move")
    {
        const TestType test_item = CreateTestItem<TestType>();
        TestType copy_item(test_item);
        CheckUnitType(UnitType<TestType>(Units::Dots, std::move(copy_item)), test_item, Units::Dots);
    }

    SECTION("Initialize with units and original type construction arguments")
    {
        CheckUnitType(CreateUnitItem<TestType>(Units::Dots), CreateTestItem<TestType>(), Units::Dots);
    }

    if constexpr (std::is_same_v<FramePoint, TestType>)
    {
        SECTION("Initialize frame point with frame size")
        {
            const UnitSize test_size = CreateUnitItem<FrameSize>(Units::Dots);
            CheckUnitType(UnitType<FramePoint>(test_size), FramePoint(test_size.GetWidth(), test_size.GetHeight()), Units::Dots);
        }
    }
}

TEMPLATE_TEST_CASE("Unit Type Conversions", "[unit][type][convert]", ALL_BASE_TYPES)
{
    SECTION("Convert to base type reference")
    {
        const TestType     base_item = CreateTestItem<TestType>();
        UnitType<TestType> unit_item = CreateUnitItem<TestType>(Units::Dots);
        CHECK(unit_item.AsBase() == base_item);
    }

    SECTION("Convert to base type const reference")
    {
        const TestType           base_item = CreateTestItem<TestType>();
        const UnitType<TestType> unit_item = CreateUnitItem<TestType>(Units::Dots);
        CHECK(unit_item.AsBase() == base_item);
    }

    SECTION("Convert Pixels type to string")
    {
        const UnitType<TestType> unit_item = CreateUnitItem<TestType>(Units::Pixels);
        const std::string unit_str = static_cast<std::string>(unit_item.AsBase()) + " in Pixels";
        CHECK(static_cast<std::string>(unit_item) == unit_str);
    }

    SECTION("Convert Dots type to string")
    {
        const UnitType<TestType> unit_item = CreateUnitItem<TestType>(Units::Dots);
        const std::string unit_str = static_cast<std::string>(unit_item.AsBase()) + " in Dots";
        CHECK(static_cast<std::string>(unit_item) == unit_str);
    }
}

TEMPLATE_TEST_CASE("Unit Rect Conversions", "[unit][rect][convert]", RECT_BASE_TYPES)
{
    using CoordType = typename TestType::CoordinateType;
    using DimType   = typename TestType::DimensionType;

    SECTION("Convert to unit origin")
    {
        const auto               unit_point = CreateUnitItem<Data::Point2T<CoordType>>(Units::Dots);
        const UnitType<TestType> unit_rect  = CreateUnitItem<TestType>(Units::Dots);
        CHECK(unit_rect.GetUnitOrigin() == unit_point);
    }

    SECTION("Convert to unit size")
    {
        const auto               unit_size = CreateUnitItem<Data::RectSize<DimType>>(Units::Dots);
        const UnitType<TestType> unit_rect = CreateUnitItem<TestType>(Units::Dots);
        CHECK(unit_rect.GetUnitSize() == unit_size);
    }
}

TEMPLATE_TEST_CASE("Unit Types Strict Comparison", "[unit][type][compare]", ALL_BASE_TYPES)
{
    const UnitType<TestType> dot_item_a = CreateUnitItem<TestType>(Units::Dots, 0);
    const UnitType<TestType> pix_item_a = CreateUnitItem<TestType>(Units::Pixels, 0);
    const UnitType<TestType> dot_item_b = CreateUnitItem<TestType>(Units::Dots, 10);

    SECTION("Equality")
    {
        CHECK(dot_item_a == CreateUnitItem<TestType>(Units::Dots, 0));
        CHECK_FALSE(dot_item_a == dot_item_b);
        CHECK_FALSE(dot_item_a == pix_item_a);
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(dot_item_a != CreateUnitItem<TestType>(Units::Dots, 0));
        CHECK(dot_item_a != dot_item_b);
        CHECK(dot_item_a != pix_item_a);
    }
}

TEMPLATE_TEST_CASE("Unit Types Non-Strict Comparison", "[unit][type][compare]", POINT_BASE_TYPES, SIZE_BASE_TYPES)
{
    const UnitType<TestType> dot_item_a = CreateUnitItem<TestType>(Units::Dots, 0);
    const UnitType<TestType> pix_item_a = CreateUnitItem<TestType>(Units::Pixels, 0);
    const UnitType<TestType> dot_item_b = CreateUnitItem<TestType>(Units::Dots, 10);
    const UnitType<TestType> pix_item_b = CreateUnitItem<TestType>(Units::Pixels, 10);

    SECTION("Less")
    {
        CHECK(dot_item_a < dot_item_b);
        CHECK(pix_item_a < pix_item_b);
        CHECK_THROWS(dot_item_a < pix_item_b);
    }

    SECTION("Less or equal")
    {
        CHECK(dot_item_a <= dot_item_b);
        CHECK(pix_item_a <= pix_item_b);
        CHECK_THROWS(dot_item_a <= pix_item_b);
    }

    SECTION("Greater")
    {
        CHECK(dot_item_b > dot_item_a);
        CHECK(pix_item_b > pix_item_a);
        CHECK_THROWS(dot_item_b > pix_item_a);
    }

    SECTION("Greater or equal")
    {
        CHECK(dot_item_b >= dot_item_a);
        CHECK(pix_item_b >= pix_item_a);
        CHECK_THROWS(dot_item_b >= pix_item_a);
    }
}