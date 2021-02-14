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

using namespace Methane::Data;
using namespace Methane::UserInterface;

#define POINT_BASE_TYPES FramePoint, FloatPoint
#define SIZE_BASE_TYPES  FrameSize,  FloatSize
#define RECT_BASE_TYPES  FrameRect,  FloatRect

#define ALL_BASE_TYPES POINT_BASE_TYPES, SIZE_BASE_TYPES, RECT_BASE_TYPES

template<typename T>
void CheckUnitType(const UnitType<Methane::Data::Point2T<T>>& unit_point, const Methane::Data::Point2T<T>& orig_point, Units units)
{
    CHECK(unit_point.GetUnits() == units);
    CHECK(unit_point.GetX() == orig_point.GetX());
    CHECK(unit_point.GetY() == orig_point.GetY());
}

template<typename T>
void CheckUnitType(const UnitType<RectSize<T>>& unit_size, const RectSize<T>& orig_size, Units units)
{
    CHECK(unit_size.GetUnits()  == units);
    CHECK(unit_size.GetWidth()  == orig_size.GetWidth());
    CHECK(unit_size.GetHeight() == orig_size.GetHeight());
}

template<typename T, typename D>
void CheckUnitType(const UnitType<Rect<T, D>>& unit_rect, const Rect<T, D>& orig_rect, Units units)
{
    CHECK(unit_rect.GetUnits()  == units);
    CHECK(unit_rect.origin.GetX() == orig_rect.origin.GetX());
    CHECK(unit_rect.origin.GetY() == orig_rect.origin.GetY());
    CHECK(unit_rect.size.GetWidth()  == orig_rect.size.GetWidth());
    CHECK(unit_rect.size.GetHeight() == orig_rect.size.GetHeight());
}

template<typename T>
void GetTestItem(Methane::Data::Point2T<T>& test_item) { test_item = Methane::Data::Point2T<T>(12, 23); }

template<typename T>
void GetTestItem(Methane::Data::RectSize<T>& test_item) { test_item = Methane::Data::RectSize<T>(123, 234); }

template<typename T, typename D>
void GetTestItem(Methane::Data::Rect<T, D>& test_item) { test_item = Methane::Data::Rect<T, D>(12, 23, 123, 234); }

template<typename TestType>
TestType GetTestItem() { TestType test_item; GetTestItem(test_item); return test_item; }

TEMPLATE_TEST_CASE("Unit Type Initialization", "[unit][point][init]", ALL_BASE_TYPES)
{
    SECTION("Default constructor initialization")
    {
        CheckUnitType(UnitType<TestType>(), TestType(), Units::Pixels);
    }

    SECTION("Initialize with a original type reference")
    {
        const TestType test_item = GetTestItem<TestType>();
        CheckUnitType(UnitType<TestType>(test_item), test_item, Units::Pixels);
    }

    SECTION("Initialize with units and original type reference")
    {
        const TestType test_item = GetTestItem<TestType>();
        CheckUnitType(UnitType<TestType>(Units::Dots, test_item), test_item, Units::Dots);
    }
}
