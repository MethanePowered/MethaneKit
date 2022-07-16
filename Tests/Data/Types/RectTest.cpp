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

FILE: Tests/Data/Types/RectTest.cpp
Unit-tests of the Rect data type

******************************************************************************/

#include <Methane/Data/Rect.hpp>
#include <Methane/Data/TypeFormatters.hpp>

#include <catch2/catch_template_test_macros.hpp>

using namespace Methane::Data;

template<typename T, size_t size>
struct Catch::StringMaker<Point<T, size>>
{
    static std::string convert(const Point<T, size>& v)
    {
        return static_cast<std::string>(v);
    }
};

template<typename D>
struct Catch::StringMaker<RectSize<D>>
{
    static std::string convert(const RectSize<D>& v)
    {
        return static_cast<std::string>(v);
    }
};

template<typename D> using RectPtInt    = Rect<int32_t, D>;
template<typename D> using RectPtUint   = Rect<uint32_t, D>;
template<typename D> using RectPtFloat  = Rect<float, D>;
template<typename D> using RectPtDouble = Rect<double, D>;

#define RECT_TYPES_PRODUCT \
    (RectPtInt, RectPtUint, RectPtFloat, RectPtDouble), \
    (int32_t,   uint32_t,   float,       double)

TEMPLATE_PRODUCT_TEST_CASE("Rectangle Initialization", "[rect][init]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;

    const PointType test_origin(123, 234);
    const SizeType  test_size(67, 89);

    SECTION("Default initialization of empty rect")
    {
        const TestType test_rect;
        CHECK(test_rect.origin == PointType());
        CHECK(test_rect.size == SizeType());
    }

    SECTION("Origin only initialization")
    {
        const TestType test_rect(test_origin);
        CHECK(test_rect.origin == test_origin);
        CHECK(test_rect.size == SizeType());
    }

    SECTION("Size only initialization")
    {
        const TestType test_rect(test_size);
        CHECK(test_rect.origin == PointType());
        CHECK(test_rect.size == test_size);
    }

    SECTION("Origin and size initialization")
    {
        const TestType test_rect(test_origin, test_size);
        CHECK(test_rect.origin == test_origin);
        CHECK(test_rect.size == test_size);
    }

    SECTION("Coordinates and dimensions initialization")
    {
        const TestType test_rect(123, 234, 67, 89);
        CHECK(test_rect.origin == test_origin);
        CHECK(test_rect.size == test_size);
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Rectangles Comparison", "[rect][compare]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;

    const PointType test_origin(123, 234);
    const SizeType  test_size(67, 89);
    const TestType  test_rect(test_origin, test_size);

    SECTION("Equality")
    {
        CHECK(test_rect == TestType(test_origin, test_size));
        CHECK_FALSE(test_rect == TestType(test_origin));
        CHECK_FALSE(test_rect == TestType(test_size));
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(test_rect != TestType(test_origin, test_size));
        CHECK(test_rect != TestType(test_origin));
        CHECK(test_rect != TestType(test_size));
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Rectangle Math Operations", "[rect][math]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;
    using CoordType = typename TestType::CoordinateType;
    using DimType   = typename TestType::DimensionType;

    const PointType test_origin(2, 4);
    const SizeType  test_size(6, 8);
    const TestType  test_rect(test_origin, test_size);

    SECTION("Multiplication by scalar of coordinate type")
    {
        const TestType res_rect = test_rect * CoordType(2);
        CHECK(res_rect.origin == PointType(4, 8));
        CHECK(res_rect.size   == SizeType(12, 16));
    }

    SECTION("Multiplication by scalar of dimension type")
    {
        const TestType res_rect = test_rect * DimType(2);
        CHECK(res_rect.origin == PointType(4, 8));
        CHECK(res_rect.size   == SizeType(12, 16));
    }

    SECTION("Division by scalar of coordinate type")
    {
        const TestType res_rect = test_rect / CoordType(2);
        CHECK(res_rect.origin == PointType(1, 2));
        CHECK(res_rect.size   == SizeType(3, 4));
    }

    SECTION("Division by scalar of dimension type")
    {
        const TestType res_rect = test_rect / DimType(2);
        CHECK(res_rect.origin == PointType(1, 2));
        CHECK(res_rect.size   == SizeType(3, 4));
    }

    SECTION("Inplace multiplication by scalar of coordinate type")
    {
        TestType res_rect = test_rect;
        res_rect *= CoordType(2);
        CHECK(res_rect.origin == PointType(4, 8));
        CHECK(res_rect.size   == SizeType(12, 16));
    }

    SECTION("Inplace multiplication by scalar of dimension type")
    {
        TestType res_rect = test_rect;
        res_rect *= DimType(2);
        CHECK(res_rect.origin == PointType(4, 8));
        CHECK(res_rect.size   == SizeType(12, 16));
    }

    SECTION("Inplace division by scalar of coordinate type")
    {
        TestType res_rect = test_rect;
        res_rect /= CoordType(2);
        CHECK(res_rect.origin == PointType(1, 2));
        CHECK(res_rect.size   == SizeType(3, 4));
    }

    SECTION("Inplace division by scalar of dimension type")
    {
        TestType res_rect = test_rect;
        res_rect /= DimType(2);
        CHECK(res_rect.origin == PointType(1, 2));
        CHECK(res_rect.size   == SizeType(3, 4));
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Rectangle Conversion to Other Types", "[rect][convert]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;
    using CoordType = typename TestType::CoordinateType;
    using DimType   = typename TestType::DimensionType;

    const PointType test_origin(123, 234);
    const SizeType  test_size(67, 89);
    const TestType  test_rect(test_origin, test_size);

    if constexpr (std::is_floating_point_v<CoordType>)
    {
        if constexpr (std::is_floating_point_v<DimType>)
        {
            SECTION("Convert to integer point and integer size rectangle")
            {
                CHECK(static_cast<Rect<int32_t, uint32_t>>(test_rect) == Rect<int32_t, uint32_t>(123, 234, 67U, 89U));
            }
        }
        else
        {
            SECTION("Convert to integer point and floating size rectangle")
            {
                CHECK(static_cast<Rect<int32_t, float>>(test_rect) == Rect<int32_t, float>(123, 234, 67.F, 89.F));
            }
        }
    }
    else
    {
        if constexpr (std::is_floating_point_v<DimType>)
        {
            SECTION("Convert to floating point and integer size rectangle")
            {
                CHECK(static_cast<Rect<float, uint32_t>>(test_rect) == Rect<float, uint32_t>(123.F, 234.F, 67U, 89U));
            }
        }
        else
        {
            SECTION("Convert to floating point and floating size rectangle")
            {
                CHECK(static_cast<Rect<float, float>>(test_rect) == Rect<float, float>(123.F, 234.F, 67.F, 89.F));
            }
        }
    }

    SECTION("Conversion to string")
    {
        CHECK(static_cast<std::string>(test_rect) == "Rect[P(123, 234) : Sz(67 x 89)]");
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Rectangle Property Getters", "[rect][accessors]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;
    using CoordType = typename TestType::CoordinateType;

    const PointType test_origin(123, 234);
    const SizeType  test_size(67, 89);
    const TestType  test_rect(test_origin, test_size);

    SECTION("Left coordinate getter")
    {
        CHECK(test_rect.GetLeft() == CoordType(123));
    }

    SECTION("Right coordinate getter")
    {
        CHECK(test_rect.GetRight() == CoordType(190));
    }

    SECTION("Top coordinate getter")
    {
        CHECK(test_rect.GetTop() == CoordType(234));
    }

    SECTION("Bottom coordinate getter")
    {
        CHECK(test_rect.GetBottom() == CoordType(323));
    }
}