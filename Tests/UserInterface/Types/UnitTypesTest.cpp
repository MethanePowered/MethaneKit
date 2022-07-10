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

#include "UnitTypeCatchHelpers.hpp"

#include <Methane/Data/TypeTraits.hpp>

#include <catch2/catch_template_test_macros.hpp>

using namespace Methane;
using namespace Methane::UserInterface;

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

    if constexpr (TypeTraits<TestType>::type_of == TypeOf::Rect)
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
}

TEMPLATE_TEST_CASE("Unit Types Comparison", "[unit][type][compare]", ALL_BASE_TYPES)
{
    const UnitType<TestType> dot_item_a = CreateUnitItem<TestType>(Units::Dots);
    const UnitType<TestType> pix_item_a = CreateUnitItem<TestType>(Units::Pixels);
    const UnitType<TestType> dot_item_b = CreateUnitItem<TestType>(Units::Dots, 2);

    SECTION("Equality")
    {
        CHECK(dot_item_a == CreateUnitItem<TestType>(Units::Dots));
        CHECK_FALSE(dot_item_a == dot_item_b);
        CHECK_FALSE(dot_item_a == pix_item_a);
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(dot_item_a != CreateUnitItem<TestType>(Units::Dots));
        CHECK(dot_item_a != dot_item_b);
        CHECK(dot_item_a != pix_item_a);
    }

    if constexpr (TypeTraits<TestType>::type_of == TypeOf::Point ||
                  TypeTraits<TestType>::type_of == TypeOf::RectSize)
    {
        const UnitType<TestType> pix_item_b = CreateUnitItem<TestType>(Units::Pixels, 2);

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
}

TEMPLATE_TEST_CASE("Unit Type Math Operations", "[unit][type][math]", ALL_BASE_TYPES)
{
    const UnitType<TestType> test_item_1dt = CreateUnitItem<TestType>(Units::Dots, 1);
    const UnitType<TestType> test_item_2dt = CreateUnitItem<TestType>(Units::Dots, 2);

    SECTION("Multiplication by scalar")
    {
        CHECK(test_item_1dt * 2 == CreateUnitItem<TestType>(Units::Dots, 2));
    }

    SECTION("Division by scalar")
    {
        CHECK(test_item_2dt / 2 == CreateUnitItem<TestType>(Units::Dots));
    }

    SECTION("Inplace multiplication by scalar")
    {
        UnitType<TestType> test_item = test_item_1dt;
        test_item *= 2;
        CHECK(test_item == CreateUnitItem<TestType>(Units::Dots, 2));
    }

    SECTION("Inplace division by scalar")
    {
        UnitType<TestType> test_item = test_item_2dt;
        test_item /= 2;
        CHECK(test_item == CreateUnitItem<TestType>(Units::Dots));
    }

    if constexpr (TypeTraits<TestType>::type_of == TypeOf::Point ||
                  TypeTraits<TestType>::type_of == TypeOf::RectSize)
    {
        const UnitType<TestType> test_item_1px = CreateUnitItem<TestType>(Units::Pixels, 1);
        const UnitType<TestType> test_item_2px = CreateUnitItem<TestType>(Units::Pixels, 2);
        const UnitType<TestType> test_item_3px = CreateUnitItem<TestType>(Units::Pixels, 3);

        SECTION("Addition")
        {
            CHECK(test_item_1px + test_item_2px == test_item_3px);
            CHECK_THROWS(test_item_1px + test_item_2dt);
        }

        SECTION("Subtraction")
        {
            CHECK(test_item_3px - test_item_1px == test_item_2px);
            CHECK_THROWS(test_item_2dt - test_item_1px);
        }

        SECTION("Inplace Addition")
        {
            UnitType<TestType> test_item_px = CreateUnitItem<TestType>(Units::Pixels, 1);
            test_item_px += test_item_2px;
            CHECK(test_item_px == test_item_3px);
            CHECK_THROWS(test_item_px += test_item_2dt);
        }

        SECTION("Inplace Subtraction")
        {
            UnitType<TestType> test_item_px = CreateUnitItem<TestType>(Units::Pixels, 3);
            test_item_px -= test_item_1px;
            CHECK(test_item_px == test_item_2px);
            CHECK_THROWS(test_item_px -= test_item_2dt);
        }
    }
}
