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

template<typename T>
void CreateTestItem(Data::Point2T<T>& test_item) { test_item = Data::Point2T<T>(T(12), T(23)); }

template<typename T>
void CreateTestItem(Data::RectSize<T>& test_item) { test_item = Methane::Data::RectSize<T>(T(123), T(234)); }

template<typename T, typename D>
void CreateTestItem(Data::Rect<T, D>& test_item) { test_item = Methane::Data::Rect<T, D>(T(12), T(23), D(123), D(234)); }

template<typename TestType>
TestType CreateTestItem() { TestType test_item; CreateTestItem(test_item); return test_item; }

template<typename T>
void CreateTestItem(Units units, UnitType<Data::Point2T<T>>& test_item) { test_item = UnitType<Methane::Data::Point2T<T>>(units, T(12), T(23)); }

template<typename T>
void CreateTestItem(Units units, UnitType<Data::RectSize<T>>& test_item) { test_item = UnitType<Methane::Data::RectSize<T>>(units, T(123), T(234)); }

template<typename T, typename D>
void CreateTestItem(Units units, UnitType<Data::Rect<T, D>>& test_item) { test_item = UnitType<Methane::Data::Rect<T, D>>(units, T(12), T(23), D(123), D(234)); }

template<typename TestType>
UnitType<TestType> CreateTestItem(Units units) { UnitType<TestType> test_item; CreateTestItem(units, test_item); return test_item; }

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
        CheckUnitType(CreateTestItem<TestType>(Units::Dots), CreateTestItem<TestType>(), Units::Dots);
    }

    if constexpr (std::is_same_v<FramePoint, TestType>)
    {
        SECTION("Initialize frame point with frame size")
        {
            const UnitSize test_size = CreateTestItem<FrameSize>(Units::Dots);
            CheckUnitType(UnitType<FramePoint>(test_size), FramePoint(test_size.GetWidth(), test_size.GetHeight()), Units::Dots);
        }
    }
}

TEMPLATE_TEST_CASE("Unit Point Conversions", "[unit][point][init]", POINT_BASE_TYPES)
{
    SECTION("Convert to base point reference")
    {
        const TestType     test_point = CreateTestItem<TestType>();
        UnitType<TestType> unit_point = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_point.AsPoint() == test_point);
    }

    SECTION("Convert to base point const reference")
    {
        const TestType           test_point = CreateTestItem<TestType>();
        const UnitType<TestType> unit_point = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_point.AsPoint() == test_point);
    }
}

TEMPLATE_TEST_CASE("Unit Size Conversions", "[unit][size][init]", SIZE_BASE_TYPES)
{
    SECTION("Convert to base size reference")
    {
        const TestType     test_size = CreateTestItem<TestType>();
        UnitType<TestType> unit_size = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_size.AsSize() == test_size);
    }

    SECTION("Convert to base size const reference")
    {
        const TestType           test_size = CreateTestItem<TestType>();
        const UnitType<TestType> unit_size = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_size.AsSize() == test_size);
    }
}

TEMPLATE_TEST_CASE("Unit Rect Conversions", "[unit][rect][init]", RECT_BASE_TYPES)
{
    SECTION("Convert to base rect reference")
    {
        const TestType     test_rect = CreateTestItem<TestType>();
        UnitType<TestType> unit_rect = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_rect.AsRect() == test_rect);
    }

    SECTION("Convert to base rect const reference")
    {
        const TestType           test_rect = CreateTestItem<TestType>();
        const UnitType<TestType> unit_rect = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_rect.AsRect() == test_rect);
    }

    SECTION("Convert to unit origin")
    {
        const auto               unit_point = CreateTestItem<Data::Point2T<typename TestType::CoordinateType>>(Units::Dots);
        const UnitType<TestType> unit_rect  = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_rect.GetUnitOrigin() == unit_point);
    }

    SECTION("Convert to unit size")
    {
        const auto               unit_size = CreateTestItem<Data::RectSize<typename TestType::DimensionType>>(Units::Dots);
        const UnitType<TestType> unit_rect = CreateTestItem<TestType>(Units::Dots);
        CHECK(unit_rect.GetUnitSize() == unit_size);
    }
}