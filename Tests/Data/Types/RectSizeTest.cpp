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

FILE: Tests/Data/Types/RectSizeTest.cpp
Unit-tests of the RectSize data type

******************************************************************************/

#include <Methane/Data/Rect.hpp>

#include <catch2/catch_template_test_macros.hpp>

using namespace Methane::Data;

template<typename D>
struct Catch::StringMaker<RectSize<D>>
{
    static std::string convert(const RectSize<D>& v)
    {
        return static_cast<std::string>(v);
    }
};

#define RECT_SIZE_TYPES int32_t, uint32_t, float, double

TEMPLATE_TEST_CASE("Rectangle Size Initialization", "[rect][size][init]", RECT_SIZE_TYPES)
{
    const TestType test_width  = TestType(123) / 2;
    const TestType test_height = TestType(235) / 3;

    SECTION("Default initialization of zero size")
    {
        const RectSize<TestType> rect_size;
        CHECK(rect_size.GetWidth()  == TestType(0));
        CHECK(rect_size.GetHeight() == TestType(0));
    }

    SECTION("Initialization with dimensions of same type")
    {
        const RectSize<TestType> rect_size(test_width, test_height);
        CHECK(rect_size.GetWidth()  == test_width);
        CHECK(rect_size.GetHeight() == test_height);
    }

    SECTION("Initialization with point of same type")
    {
        const Point2T<TestType> test_point(test_width, test_height);
        const RectSize<TestType> rect_size(test_point);
        CHECK(rect_size.GetWidth()  == test_width);
        CHECK(rect_size.GetHeight() == test_height);
    }

    if constexpr (std::is_signed_v<TestType>)
    {
        SECTION("Exception on initialization with negative dimensions")
        {
            CHECK_THROWS_AS(RectSize<TestType>(-test_width, test_height), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(RectSize<TestType>(test_width, -test_height), Methane::ArgumentExceptionBase<std::out_of_range>);
        }

        SECTION("Exception on initialization with negative point coordinates")
        {
            CHECK_THROWS_AS(RectSize<TestType>(Point2T<TestType>(-test_width, test_height)), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(RectSize<TestType>(Point2T<TestType>(test_width, -test_height)), Methane::ArgumentExceptionBase<std::out_of_range>);
        }
    }

    if constexpr (std::is_floating_point_v<TestType>)
    {
        const uint32_t ui_width  = 123U;
        const uint32_t ui_height = 567U;

        SECTION("Initialization with dimensions of integer type")
        {
            const RectSize<TestType> rect_size(ui_width, ui_height);
            CHECK(rect_size.GetWidth()  == static_cast<TestType>(ui_width));
            CHECK(rect_size.GetHeight() == static_cast<TestType>(ui_height));
        }

        SECTION("Initialization with point of integer type")
        {
            const Point2U test_point(ui_width, ui_height);
            const RectSize<TestType> rect_size(test_point);
            CHECK(rect_size.GetWidth()  == static_cast<TestType>(ui_width));
            CHECK(rect_size.GetHeight() == static_cast<TestType>(ui_height));
        }
    }
    else
    {
        const float fp_width  = 1.23F;
        const float fp_height = 5.67F;

        SECTION("Dimensions with dimensions of floating point type")
        {
            const RectSize<TestType> rect_size(fp_width, fp_height);
            CHECK(rect_size.GetWidth()  == TestType(1));
            CHECK(rect_size.GetHeight() == TestType(6));
        }

        SECTION("Initialization with point of floating point type")
        {
            const Point2F test_point(fp_width, fp_height);
            const RectSize<TestType> rect_size(test_point);
            CHECK(rect_size.GetWidth()  == TestType(1));
            CHECK(rect_size.GetHeight() == TestType(6));
        }
    }

    SECTION("Maximum rectangle initialization")
    {
        const RectSize<TestType> rect_size = RectSize<TestType>::Max();
        CHECK(rect_size.GetWidth()  == std::numeric_limits<TestType>::max());
        CHECK(rect_size.GetHeight() == std::numeric_limits<TestType>::max());
    }

    SECTION("Copy constructor initialization")
    {
        const RectSize<TestType> orig_size(test_width, test_height);
        const RectSize<TestType> copy_size(orig_size);
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
    }

    SECTION("Move constructor initialization")
    {
        RectSize<TestType> orig_size(test_width, test_height);
        const RectSize<TestType> copy_size(std::move(orig_size));
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
    }

    SECTION("Copy assignment initialization")
    {
        const RectSize<TestType> orig_size(test_width, test_height);
        RectSize<TestType> copy_size;
        copy_size = orig_size;
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
    }

    SECTION("Move assignment initialization")
    {
        RectSize<TestType> orig_size(test_width, test_height);
        RectSize<TestType> copy_size;
        copy_size = std::move(orig_size);
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
    }
}

TEMPLATE_TEST_CASE("Rectangle Size Comparison", "[rect][size][compare]", RECT_SIZE_TYPES)
{
    const TestType big_width   = TestType(123);
    const TestType big_height  = TestType(235);
    const TestType small_width  = big_width  / 2;
    const TestType small_height = big_height / 3;
    const RectSize<TestType> small_size(small_width, small_height);

    SECTION("Equality")
    {
        CHECK(small_size == RectSize<TestType>(small_width, small_height));
        CHECK_FALSE(small_size == RectSize<TestType>(small_width, small_width));
        CHECK_FALSE(small_size == RectSize<TestType>(small_height, small_height));
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(small_size != RectSize<TestType>(small_width, small_height));
        CHECK(small_size != RectSize<TestType>(small_width, small_width));
        CHECK(small_size != RectSize<TestType>(small_height, small_height));
    }

    SECTION("Less")
    {
        CHECK_FALSE(small_size < RectSize<TestType>(small_width, small_height));
        CHECK_FALSE(small_size < RectSize<TestType>(small_width, big_height));
        CHECK(small_size < RectSize<TestType>(big_width, big_height));
    }

    SECTION("Less or equal")
    {
        CHECK(small_size <= RectSize<TestType>(small_width, small_height));
        CHECK(small_size <= RectSize<TestType>(small_width, big_height));
        CHECK(small_size <= RectSize<TestType>(big_width, big_height));
        CHECK_FALSE(RectSize<TestType>(big_width, big_height) <= small_size);
    }

    SECTION("Greater")
    {
        CHECK_FALSE(RectSize<TestType>(small_width, small_height) > small_size);
        CHECK_FALSE(RectSize<TestType>(small_width, big_height) > small_size);
        CHECK(RectSize<TestType>(big_width, big_height) > small_size);
    }

    SECTION("Greater or equal")
    {
        CHECK(RectSize<TestType>(small_width, small_height) >= small_size);
        CHECK(RectSize<TestType>(small_width, big_height) >= small_size);
        CHECK(RectSize<TestType>(big_width, big_height) >= small_size);
        CHECK_FALSE(small_size >= RectSize<TestType>(big_width, big_height));
    }
}

TEMPLATE_TEST_CASE("Rectangle Size Math Operations", "[rect][size][math]", RECT_SIZE_TYPES)
{
    const TestType big_width   = TestType(123);
    const TestType big_height  = TestType(235);
    const TestType small_width  = big_width  / 2;
    const TestType small_height = big_height / 3;
    const RectSize<TestType> small_size(small_width, small_height);
    const RectSize<TestType> big_size(big_width, big_height);

    SECTION("Addition of size with same type")
    {
        const RectSize<TestType> res_size = big_size + small_size;
        CHECK(res_size.GetWidth()  == big_width + small_width);
        CHECK(res_size.GetHeight() == big_height + small_height);
    }

    SECTION("Subtraction of size with same type")
    {
        const RectSize<TestType> res_size = big_size - small_size;
        CHECK(res_size.GetWidth()  == big_width - small_width);
        CHECK(res_size.GetHeight() == big_height - small_height);
    }

    SECTION("Inplace addition of size with same type")
    {
        RectSize<TestType> res_size = big_size;
        res_size += small_size;
        CHECK(res_size.GetWidth()  == big_width + small_width);
        CHECK(res_size.GetHeight() == big_height + small_height);
    }

    SECTION("Inplace subtraction of size with same type")
    {
        RectSize<TestType> res_size = big_size;
        res_size -= small_size;
        CHECK(res_size.GetWidth()  == big_width - small_width);
        CHECK(res_size.GetHeight() == big_height - small_height);
    }

    const TestType multiplier(2);

    SECTION("Multiplication by scalar of same type")
    {
        const RectSize<TestType> res_size = small_size * multiplier;
        CHECK(res_size.GetWidth()  == small_width  * multiplier);
        CHECK(res_size.GetHeight() == small_height * multiplier);
    }

    SECTION("Division by scalar of same type")
    {
        const RectSize<TestType> res_size = big_size / multiplier;
        CHECK(res_size.GetWidth()  == big_width  / multiplier);
        CHECK(res_size.GetHeight() == big_height / multiplier);
    }

    SECTION("Inplace multiplication by scalar of same type")
    {
        RectSize<TestType> res_size = small_size;
        res_size *= multiplier;
        CHECK(res_size.GetWidth()  == small_width  * multiplier);
        CHECK(res_size.GetHeight() == small_height * multiplier);
    }

    SECTION("Inplace division by scalar of same type")
    {
        RectSize<TestType> res_size = big_size;
        res_size /= multiplier;
        CHECK(res_size.GetWidth()  == big_width  / multiplier);
        CHECK(res_size.GetHeight() == big_height / multiplier);
    }

    const RectSize<TestType> multiplier_size(TestType(2), TestType(3));

    SECTION("Multiplication by size of same type")
    {
        const RectSize<TestType> res_size = small_size * multiplier_size;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == small_height * multiplier_size.GetHeight());
    }

    SECTION("Division by size of same type")
    {
        const RectSize<TestType> res_size = big_size / multiplier_size;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == big_height / multiplier_size.GetHeight());
    }

    SECTION("Inplace multiplication by size of same type")
    {
        RectSize<TestType> res_size = small_size;
        res_size *= multiplier_size;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == small_height * multiplier_size.GetHeight());
    }

    SECTION("Inplace division by size of same type")
    {
        RectSize<TestType> res_size = big_size;
        res_size /= multiplier_size;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == big_height / multiplier_size.GetHeight());
    }

    const Point2T<TestType> multiplier_point(TestType(2), TestType(3));

    SECTION("Multiplication by point of same type")
    {
        const RectSize<TestType> res_size = small_size * multiplier_point;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_point.GetX());
        CHECK(res_size.GetHeight() == small_height * multiplier_point.GetY());
    }

    SECTION("Division by point of same type")
    {
        const RectSize<TestType> res_size = big_size / multiplier_point;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_point.GetX());
        CHECK(res_size.GetHeight() == big_height / multiplier_point.GetY());
    }

    SECTION("Inplace multiplication by point of same type")
    {
        RectSize<TestType> res_size = small_size;
        res_size *= multiplier_point;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_point.GetX());
        CHECK(res_size.GetHeight() == small_height * multiplier_point.GetY());
    }

    SECTION("Inplace division by point of same type")
    {
        RectSize<TestType> res_size = big_size;
        res_size /= multiplier_point;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_point.GetX());
        CHECK(res_size.GetHeight() == big_height / multiplier_point.GetY());
    }

    if constexpr (std::is_floating_point_v<TestType>)
    {
        const uint32_t ui_multiplier = 2U;

        SECTION("Multiplication by scalar of integer type")
        {
            const RectSize<TestType> res_size = small_size * ui_multiplier;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier));
        }

        SECTION("Division by scalar of integer type")
        {
            const RectSize<TestType> res_size = big_size / ui_multiplier;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier));
        }

        SECTION("Inplace multiplication by scalar of integer type")
        {
            RectSize<TestType> res_size = small_size;
            res_size *= ui_multiplier;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier));
        }

        SECTION("Inplace division by scalar of integer type")
        {
            RectSize<TestType> res_size = big_size;
            res_size /= ui_multiplier;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier));
        }

        const RectSize<uint32_t> ui_multiplier_size(2U, 3U);

        SECTION("Multiplication by size of integer type")
        {
            const RectSize<TestType> res_size = small_size * ui_multiplier_size;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_size.GetHeight()));
        }

        SECTION("Division by size of integer type")
        {
            const RectSize<TestType> res_size = big_size / ui_multiplier_size;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_size.GetHeight()));
        }

        SECTION("Inplace multiplication by size of integer type")
        {
            RectSize<TestType> res_size = small_size;
            res_size *= ui_multiplier_size;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_size.GetHeight()));
        }

        SECTION("Inplace division by size of integer type")
        {
            RectSize<TestType> res_size = big_size;
            res_size /= ui_multiplier_size;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_size.GetHeight()));
        }

        const Point2T<uint32_t> ui_multiplier_point(2U, 3U);

        SECTION("Multiplication by point of integer type")
        {
            const RectSize<TestType> res_size = small_size * ui_multiplier_point;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_point.GetY()));
        }

        SECTION("Division by point of integer type")
        {
            const RectSize<TestType> res_size = big_size / ui_multiplier_point;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_point.GetY()));
        }

        SECTION("Inplace multiplication by point of integer type")
        {
            RectSize<TestType> res_size = small_size;
            res_size *= ui_multiplier_point;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_point.GetY()));
        }

        SECTION("Inplace division by point of integer type")
        {
            RectSize<TestType> res_size = big_size;
            res_size /= ui_multiplier_point;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_point.GetY()));
        }
    }
    else
    {
        const float fp_multiplier = 2.4F;

        SECTION("Multiplication by scalar of floating point type")
        {
            const RectSize<TestType> res_size = small_size * fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier)));
        }

        SECTION("Division by scalar of floating point type")
        {
            const RectSize<TestType> res_size = big_size / fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier)));
        }

        SECTION("Inplace multiplication by scalar of floating point type")
        {
            RectSize<TestType> res_size = small_size;
            res_size *= fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier)));
        }

        SECTION("Inplace division by scalar of floating point type")
        {
            RectSize<TestType> res_size = big_size;
            res_size /= fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier)));
        }

        const RectSize<float> fp_multiplier_size(2.4F, 3.4F);

        SECTION("Multiplication by size of floating point type")
        {
            const RectSize<TestType> res_size = small_size * fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_size.GetHeight())));
        }

        SECTION("Division by size of floating point type")
        {
            const RectSize<TestType> res_size = big_size / fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_size.GetHeight())));
        }

        SECTION("Inplace multiplication by size of floating point type")
        {
            RectSize<TestType> res_size = small_size;
            res_size *= fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_size.GetHeight())));
        }

        SECTION("Inplace division by size of floating point type")
        {
            RectSize<TestType> res_size = big_size;
            res_size /= fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_size.GetHeight())));
        }

        const Point2T<float> fp_multiplier_point(2.6F, 3.6F);

        SECTION("Multiplication by point of floating point type")
        {
            const RectSize<TestType> res_size = small_size * fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_point.GetY())));
        }

        SECTION("Division by point of floating point type")
        {
            const RectSize<TestType> res_size = big_size / fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_point.GetY())));
        }

        SECTION("Inplace multiplication by point of floating point type")
        {
            RectSize<TestType> res_size = small_size;
            res_size *= fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_point.GetY())));
        }

        SECTION("Inplace division by point of floating point type")
        {
            RectSize<TestType> res_size = big_size;
            res_size /= fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_point.GetY())));
        }
    }
}

TEMPLATE_TEST_CASE("Rectangle Size Conversion to Other Types", "[rect][size][convert]", RECT_SIZE_TYPES)
{
    if constexpr (std::is_floating_point_v<TestType>)
    {
        const RectSize<TestType> float_test_size(1.6F, 2.4F);

        SECTION("Conversion to integer size")
        {
            CHECK(static_cast<RectSize<uint32_t>>(float_test_size) == RectSize<uint32_t>(2, 2));
        }

        SECTION("Conversion to integer point")
        {
            CHECK(static_cast<Point<uint32_t, 2>>(float_test_size) == Point<uint32_t, 2>(2, 2));
        }
    }
    else
    {
        const RectSize<TestType> int_test_size(1, 2);

        SECTION("Conversion to floating point size")
        {
            CHECK(static_cast<RectSize<float>>(int_test_size) == RectSize<float>(1.F, 2.F));
        }

        SECTION("Conversion to floating point point")
        {
            CHECK(static_cast<Point<float, 2>>(int_test_size) == Point<float, 2>(1.F, 2.F));
        }
    }

    const RectSize<TestType> test_size(TestType(1), TestType(2));

    SECTION("Conversion to floating point point")
    {
        CHECK(static_cast<Point<TestType, 2>>(test_size) == Point<TestType, 2>(TestType(1), TestType(2)));
    }

    SECTION("Conversion to boolean")
    {
        CHECK_FALSE(static_cast<bool>(RectSize<TestType>()));
        CHECK_FALSE(static_cast<bool>(RectSize<TestType>(TestType(1), TestType(0))));
        CHECK(static_cast<bool>(test_size));
    }

    SECTION("Conversion to string")
    {
        CHECK(static_cast<std::string>(test_size) == "Sz(1 x 2)");
    }
}

TEMPLATE_TEST_CASE("Rectangle Size Property Accessors", "[rect][size][accessors]", RECT_SIZE_TYPES)
{
    const TestType test_width  = TestType(123) / 2;
    const TestType test_height = TestType(235) / 3;
    const RectSize<TestType> test_size(test_width, test_height);

    SECTION("Width accessors")
    {
        RectSize<TestType> rect_size;
        CHECK_NOTHROW(rect_size.SetWidth(123));
        CHECK(rect_size.GetWidth() == 123);
        CHECK(rect_size == RectSize<TestType>(123, 0));

        if constexpr (std::is_signed_v<TestType>)
        {
            CHECK_THROWS_AS(rect_size.SetWidth(-123), Methane::ArgumentExceptionBase<std::out_of_range>);
        }
    }

    SECTION("Height accessors")
    {
        RectSize<TestType> rect_size;
        CHECK_NOTHROW(rect_size.SetHeight(235));
        CHECK(rect_size.GetHeight() == 235);
        CHECK(rect_size == RectSize<TestType>(0, 235));

        if constexpr (std::is_signed_v<TestType>)
        {
            CHECK_THROWS_AS(rect_size.SetHeight(-235), Methane::ArgumentExceptionBase<std::out_of_range>);
        }
    }

    SECTION("Get pixels count")
    {
        CHECK(test_size.GetPixelsCount() == test_width * test_height);
    }

    SECTION("Get longest side")
    {
        CHECK(test_size.GetLongestSide() == std::max(test_width, test_height));
    }
}