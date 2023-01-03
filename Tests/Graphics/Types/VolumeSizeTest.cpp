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

FILE: Tests/Graphics/Types/VolumeSizeTest.cpp
Unit-tests of the VolumeSize data type

******************************************************************************/

#include <Methane/Graphics/Volume.hpp>

#include <catch2/catch_template_test_macros.hpp>

#include <algorithm>

using namespace Methane::Graphics;

#define VOLUME_SIZE_TYPES int32_t, uint32_t, float, double

TEMPLATE_TEST_CASE("Volume Size Initialization", "[volume][size][init]", VOLUME_SIZE_TYPES)
{
    const TestType test_width  = TestType(123) / 2;
    const TestType test_height = TestType(235) / 3;
    const TestType test_depth  = TestType(345) / 4;

    SECTION("Default initialization of zero size")
    {
        const VolumeSize<TestType> vol_size;
        CHECK(vol_size.GetWidth()  == TestType(0));
        CHECK(vol_size.GetHeight() == TestType(0));
        CHECK(vol_size.GetDepth()  == TestType(1));
    }

    SECTION("Initialization with dimensions of same type")
    {
        const VolumeSize<TestType> vol_size(test_width, test_height, test_depth);
        CHECK(vol_size.GetWidth()  == test_width);
        CHECK(vol_size.GetHeight() == test_height);
        CHECK(vol_size.GetDepth()  == test_depth);
    }

    SECTION("Initialization with rect size and depth")
    {
        const VolumeSize<TestType> vol_size(RectSize<TestType>(test_width, test_height), test_depth);
        CHECK(vol_size.GetWidth()  == test_width);
        CHECK(vol_size.GetHeight() == test_height);
        CHECK(vol_size.GetDepth()  == test_depth);
    }

    SECTION("Initialization with 3D point of same type")
    {
        const VolumeSize<TestType> vol_size(Point3T<TestType>(test_width, test_height, test_depth));
        CHECK(vol_size.GetWidth()  == test_width);
        CHECK(vol_size.GetHeight() == test_height);
        CHECK(vol_size.GetDepth()  == test_depth);
    }

    if constexpr (std::is_signed_v<TestType>)
    {
        SECTION("Exception on initialization with negative dimensions")
        {
            CHECK_THROWS_AS(VolumeSize<TestType>(-test_width, test_height,  test_depth), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(VolumeSize<TestType>(test_width, -test_height,  test_depth), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(VolumeSize<TestType>(test_width,  test_height, -test_depth), Methane::ArgumentExceptionBase<std::out_of_range>);
        }

        SECTION("Exception on initialization with negative rect size or depth")
        {
            CHECK_THROWS_AS(VolumeSize<TestType>(VolumeSize<TestType>(-test_width,  test_height),  test_depth), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(VolumeSize<TestType>(VolumeSize<TestType>( test_width, -test_height),  test_depth), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(VolumeSize<TestType>(VolumeSize<TestType>( test_width,  test_height), -test_depth), Methane::ArgumentExceptionBase<std::out_of_range>);
        }

        SECTION("Exception on initialization with negative 3D point coordinates")
        {
            CHECK_THROWS_AS(VolumeSize<TestType>(Point3T<TestType>(-test_width,  test_height,  test_depth)), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(VolumeSize<TestType>(Point3T<TestType>( test_width, -test_height,  test_depth)), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(VolumeSize<TestType>(Point3T<TestType>( test_width,  test_height, -test_depth)), Methane::ArgumentExceptionBase<std::out_of_range>);
        }
    }

    if constexpr (std::is_floating_point_v<TestType>)
    {
        const uint32_t ui_width  = 123U;
        const uint32_t ui_height = 567U;
        const uint32_t ui_depth  = 678U;

        SECTION("Initialization with dimensions of integer type")
        {
            const VolumeSize<TestType> vol_size(ui_width, ui_height, ui_depth);
            CHECK(vol_size.GetWidth()  == static_cast<TestType>(ui_width));
            CHECK(vol_size.GetHeight() == static_cast<TestType>(ui_height));
            CHECK(vol_size.GetDepth()  == static_cast<TestType>(ui_depth));
        }

        SECTION("Initialization with rect size and depth of integer type")
        {
            const VolumeSize<TestType> vol_size(VolumeSize<uint32_t>(ui_width, ui_height), ui_depth);
            CHECK(vol_size.GetWidth()  == static_cast<TestType>(ui_width));
            CHECK(vol_size.GetHeight() == static_cast<TestType>(ui_height));
            CHECK(vol_size.GetDepth()  == static_cast<TestType>(ui_depth));
        }

        SECTION("Initialization with 3D point of integer type")
        {
            const VolumeSize<TestType> vol_size(Point3U(ui_width, ui_height, ui_depth));
            CHECK(vol_size.GetWidth()  == static_cast<TestType>(ui_width));
            CHECK(vol_size.GetHeight() == static_cast<TestType>(ui_height));
            CHECK(vol_size.GetDepth()  == static_cast<TestType>(ui_depth));
        }
    }
    else
    {
        const float fp_width  = 1.23F;
        const float fp_height = 5.67F;
        const float fp_depth  = 7.89F;

        SECTION("Dimensions with dimensions of floating point type")
        {
            const VolumeSize<TestType> vol_size(fp_width, fp_height, fp_depth);
            CHECK(vol_size.GetWidth()  == TestType(1));
            CHECK(vol_size.GetHeight() == TestType(6));
            CHECK(vol_size.GetDepth()  == TestType(8));
        }

        SECTION("Initialization with rect size and depth of floating point type")
        {
            const VolumeSize<TestType> vol_size(VolumeSize<float>(fp_width, fp_height), fp_depth);
            CHECK(vol_size.GetWidth()  == TestType(1));
            CHECK(vol_size.GetHeight() == TestType(6));
            CHECK(vol_size.GetDepth()  == TestType(8));
        }

        SECTION("Initialization with 3D point of floating point type")
        {
            const VolumeSize<TestType> vol_size(Point3F(fp_width, fp_height, fp_depth));
            CHECK(vol_size.GetWidth()  == TestType(1));
            CHECK(vol_size.GetHeight() == TestType(6));
            CHECK(vol_size.GetDepth()  == TestType(8));
        }
    }

    SECTION("Maximum volume initialization")
    {
        const VolumeSize<TestType> vol_size = VolumeSize<TestType>::Max();
        CHECK(vol_size.GetWidth()  == std::numeric_limits<TestType>::max());
        CHECK(vol_size.GetHeight() == std::numeric_limits<TestType>::max());
        CHECK(vol_size.GetDepth()  == std::numeric_limits<TestType>::max());
    }

    SECTION("Copy constructor initialization")
    {
        const VolumeSize<TestType> orig_size(test_width, test_height, test_depth);
        const VolumeSize<TestType> copy_size(orig_size);
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
        CHECK(copy_size.GetDepth()  == test_depth);
    }

    SECTION("Move constructor initialization")
    {
        VolumeSize<TestType> orig_size(test_width, test_height, test_depth);
        const VolumeSize<TestType> copy_size(std::move(orig_size));
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
        CHECK(copy_size.GetDepth()  == test_depth);
    }

    SECTION("Copy assignment initialization")
    {
        const VolumeSize<TestType> orig_size(test_width, test_height, test_depth);
        VolumeSize<TestType> copy_size;
        copy_size = orig_size;
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
        CHECK(copy_size.GetDepth()  == test_depth);
    }

    SECTION("Move assignment initialization")
    {
        VolumeSize<TestType> orig_size(test_width, test_height, test_depth);
        VolumeSize<TestType> copy_size;
        copy_size = std::move(orig_size);
        CHECK(copy_size.GetWidth()  == test_width);
        CHECK(copy_size.GetHeight() == test_height);
        CHECK(copy_size.GetDepth()  == test_depth);
    }
}

TEMPLATE_TEST_CASE("Volume Size Comparison", "[volume][size][compare]", VOLUME_SIZE_TYPES)
{
    const TestType big_width    = TestType(123);
    const TestType big_height   = TestType(235);
    const TestType big_depth    = TestType(345);
    const TestType small_width  = big_width  / 2;
    const TestType small_height = big_height / 3;
    const TestType small_depth  = big_depth  / 3;
    const VolumeSize<TestType> small_size(small_width, small_height, small_depth);

    SECTION("Equality")
    {
        CHECK(small_size == VolumeSize<TestType>(small_width, small_height, small_depth));
        CHECK_FALSE(small_size == VolumeSize<TestType>(small_width, small_width, small_width));
        CHECK_FALSE(small_size == VolumeSize<TestType>(small_height, small_height, small_height));
        CHECK_FALSE(small_size == VolumeSize<TestType>(small_depth, small_depth, small_depth));
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(small_size != VolumeSize<TestType>(small_width, small_height, small_depth));
        CHECK(small_size != VolumeSize<TestType>(small_width, small_width, small_width));
        CHECK(small_size != VolumeSize<TestType>(small_height, small_height, small_height));
        CHECK(small_size != VolumeSize<TestType>(small_depth, small_depth, small_depth));
    }

    SECTION("Less")
    {
        CHECK_FALSE(small_size < VolumeSize<TestType>(small_width, small_height, small_depth));
        CHECK_FALSE(small_size < VolumeSize<TestType>(big_width, big_height, small_depth));
        CHECK(small_size < VolumeSize<TestType>(big_width, big_height, big_depth));
    }

    SECTION("Less or equal")
    {
        CHECK(small_size <= VolumeSize<TestType>(small_width, small_height, small_depth));
        CHECK(small_size <= VolumeSize<TestType>(big_width, big_height, small_depth));
        CHECK_FALSE(VolumeSize<TestType>(big_width, big_height, big_depth) <= small_size);
    }

    SECTION("Greater")
    {
        CHECK_FALSE(VolumeSize<TestType>(small_width, small_height, small_depth) > small_size);
        CHECK_FALSE(VolumeSize<TestType>(big_width, big_height, small_depth) > small_size);
        CHECK(VolumeSize<TestType>(big_width, big_height, big_depth) > small_size);
    }

    SECTION("Greater or equal")
    {
        CHECK(VolumeSize<TestType>(small_width, small_height, small_depth) >= small_size);
        CHECK(VolumeSize<TestType>(big_width, big_height, small_depth) >= small_size);
        CHECK_FALSE(small_size >= VolumeSize<TestType>(big_width, big_height, big_depth));
    }
}

TEMPLATE_TEST_CASE("Volume Size Math Operations", "[volume][size][math]", VOLUME_SIZE_TYPES)
{
    const TestType big_width    = TestType(123);
    const TestType big_height   = TestType(235);
    const TestType big_depth    = TestType(345);
    const TestType small_width  = big_width  / 2;
    const TestType small_height = big_height / 3;
    const TestType small_depth  = big_depth  / 3;
    const VolumeSize<TestType> small_size(small_width, small_height, small_depth);
    const VolumeSize<TestType> big_size(big_width, big_height, big_depth);

    SECTION("Addition of size with same type")
    {
        const VolumeSize<TestType> res_size = big_size + small_size;
        CHECK(res_size.GetWidth()  == big_width  + small_width);
        CHECK(res_size.GetHeight() == big_height + small_height);
        CHECK(res_size.GetDepth()  == big_depth  + small_depth);
    }

    SECTION("Subtraction of size with same type")
    {
        const VolumeSize<TestType> res_size = big_size - small_size;
        CHECK(res_size.GetWidth()  == big_width  - small_width);
        CHECK(res_size.GetHeight() == big_height - small_height);
        CHECK(res_size.GetDepth()  == big_depth  - small_depth);
    }

    SECTION("Inplace addition of size with same type")
    {
        VolumeSize<TestType> res_size = big_size;
        res_size += small_size;
        CHECK(res_size.GetWidth()  == big_width  + small_width);
        CHECK(res_size.GetHeight() == big_height + small_height);
        CHECK(res_size.GetDepth()  == big_depth  + small_depth);
    }

    SECTION("Inplace subtraction of size with same type")
    {
        VolumeSize<TestType> res_size = big_size;
        res_size -= small_size;
        CHECK(res_size.GetWidth()  == big_width  - small_width);
        CHECK(res_size.GetHeight() == big_height - small_height);
        CHECK(res_size.GetDepth()  == big_depth  - small_depth);
    }

    const TestType multiplier(2);

    SECTION("Multiplication by scalar of same type")
    {
        const VolumeSize<TestType> res_size = small_size * multiplier;
        CHECK(res_size.GetWidth()  == small_width  * multiplier);
        CHECK(res_size.GetHeight() == small_height * multiplier);
        CHECK(res_size.GetDepth()  == small_depth  * multiplier);
    }

    SECTION("Division by scalar of same type")
    {
        const VolumeSize<TestType> res_size = big_size / multiplier;
        CHECK(res_size.GetWidth()  == big_width  / multiplier);
        CHECK(res_size.GetHeight() == big_height / multiplier);
        CHECK(res_size.GetDepth()  == big_depth  / multiplier);
    }

    SECTION("Inplace multiplication by scalar of same type")
    {
        VolumeSize<TestType> res_size = small_size;
        res_size *= multiplier;
        CHECK(res_size.GetWidth()  == small_width  * multiplier);
        CHECK(res_size.GetHeight() == small_height * multiplier);
        CHECK(res_size.GetDepth()  == small_depth  * multiplier);
    }

    SECTION("Inplace division by scalar of same type")
    {
        VolumeSize<TestType> res_size = big_size;
        res_size /= multiplier;
        CHECK(res_size.GetWidth()  == big_width  / multiplier);
        CHECK(res_size.GetHeight() == big_height / multiplier);
        CHECK(res_size.GetDepth()  == big_depth  / multiplier);
    }

    const VolumeSize<TestType> multiplier_size(TestType(2), TestType(3), TestType(4));

    SECTION("Multiplication by size of same type")
    {
        const VolumeSize<TestType> res_size = small_size * multiplier_size;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == small_height * multiplier_size.GetHeight());
        CHECK(res_size.GetDepth()  == small_depth  * multiplier_size.GetDepth());
    }

    SECTION("Division by size of same type")
    {
        const VolumeSize<TestType> res_size = big_size / multiplier_size;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == big_height / multiplier_size.GetHeight());
        CHECK(res_size.GetDepth()  == big_depth  / multiplier_size.GetDepth());
    }

    SECTION("Inplace multiplication by size of same type")
    {
        VolumeSize<TestType> res_size = small_size;
        res_size *= multiplier_size;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == small_height * multiplier_size.GetHeight());
        CHECK(res_size.GetDepth()  == small_depth  * multiplier_size.GetDepth());
    }

    SECTION("Inplace division by size of same type")
    {
        VolumeSize<TestType> res_size = big_size;
        res_size /= multiplier_size;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_size.GetWidth());
        CHECK(res_size.GetHeight() == big_height / multiplier_size.GetHeight());
        CHECK(res_size.GetDepth()  == big_depth  / multiplier_size.GetDepth());
    }

    const Point3T<TestType> multiplier_point(TestType(2), TestType(3), TestType(4));

    SECTION("Multiplication by point of same type")
    {
        const VolumeSize<TestType> res_size = small_size * multiplier_point;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_point.GetX());
        CHECK(res_size.GetHeight() == small_height * multiplier_point.GetY());
        CHECK(res_size.GetDepth()  == small_depth  * multiplier_point.GetZ());
    }

    SECTION("Division by point of same type")
    {
        const VolumeSize<TestType> res_size = big_size / multiplier_point;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_point.GetX());
        CHECK(res_size.GetHeight() == big_height / multiplier_point.GetY());
        CHECK(res_size.GetDepth()  == big_depth  / multiplier_point.GetZ());
    }

    SECTION("Inplace multiplication by point of same type")
    {
        VolumeSize<TestType> res_size = small_size;
        res_size *= multiplier_point;
        CHECK(res_size.GetWidth()  == small_width  * multiplier_point.GetX());
        CHECK(res_size.GetHeight() == small_height * multiplier_point.GetY());
        CHECK(res_size.GetDepth()  == small_depth  * multiplier_point.GetZ());
    }

    SECTION("Inplace division by point of same type")
    {
        VolumeSize<TestType> res_size = big_size;
        res_size /= multiplier_point;
        CHECK(res_size.GetWidth()  == big_width  / multiplier_point.GetX());
        CHECK(res_size.GetHeight() == big_height / multiplier_point.GetY());
        CHECK(res_size.GetDepth()  == big_depth  / multiplier_point.GetZ());
    }

    if constexpr (std::is_floating_point_v<TestType>)
    {
        const uint32_t ui_multiplier = 2U;

        SECTION("Multiplication by scalar of integer type")
        {
            const VolumeSize<TestType> res_size = small_size * ui_multiplier;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetDepth()  == small_depth  * static_cast<TestType>(ui_multiplier));
        }

        SECTION("Division by scalar of integer type")
        {
            const VolumeSize<TestType> res_size = big_size / ui_multiplier;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetDepth()  == big_depth  / static_cast<TestType>(ui_multiplier));
        }

        SECTION("Inplace multiplication by scalar of integer type")
        {
            VolumeSize<TestType> res_size = small_size;
            res_size *= ui_multiplier;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetDepth()  == small_depth  * static_cast<TestType>(ui_multiplier));
        }

        SECTION("Inplace division by scalar of integer type")
        {
            VolumeSize<TestType> res_size = big_size;
            res_size /= ui_multiplier;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier));
            CHECK(res_size.GetDepth()  == big_depth  / static_cast<TestType>(ui_multiplier));
        }

        const VolumeSize<uint32_t> ui_multiplier_size(2U, 3U, 4U);

        SECTION("Multiplication by size of integer type")
        {
            const VolumeSize<TestType> res_size = small_size * ui_multiplier_size;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_size.GetHeight()));
            CHECK(res_size.GetDepth()  == small_depth  * static_cast<TestType>(ui_multiplier_size.GetDepth()));
        }

        SECTION("Division by size of integer type")
        {
            const VolumeSize<TestType> res_size = big_size / ui_multiplier_size;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_size.GetHeight()));
            CHECK(res_size.GetDepth()  == big_depth  / static_cast<TestType>(ui_multiplier_size.GetDepth()));
        }

        SECTION("Inplace multiplication by size of integer type")
        {
            VolumeSize<TestType> res_size = small_size;
            res_size *= ui_multiplier_size;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_size.GetHeight()));
            CHECK(res_size.GetDepth()  == small_depth  * static_cast<TestType>(ui_multiplier_size.GetDepth()));
        }

        SECTION("Inplace division by size of integer type")
        {
            VolumeSize<TestType> res_size = big_size;
            res_size /= ui_multiplier_size;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_size.GetWidth()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_size.GetHeight()));
            CHECK(res_size.GetDepth()  == big_depth  / static_cast<TestType>(ui_multiplier_size.GetDepth()));
        }

        const Point3U ui_multiplier_point(2U, 3U, 4U);

        SECTION("Multiplication by point of integer type")
        {
            const VolumeSize<TestType> res_size = small_size * ui_multiplier_point;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_point.GetY()));
            CHECK(res_size.GetDepth()  == small_depth  * static_cast<TestType>(ui_multiplier_point.GetZ()));
        }

        SECTION("Division by point of integer type")
        {
            const VolumeSize<TestType> res_size = big_size / ui_multiplier_point;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_point.GetY()));
            CHECK(res_size.GetDepth()  == big_depth  / static_cast<TestType>(ui_multiplier_point.GetZ()));
        }

        SECTION("Inplace multiplication by point of integer type")
        {
            VolumeSize<TestType> res_size = small_size;
            res_size *= ui_multiplier_point;
            CHECK(res_size.GetWidth()  == small_width  * static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == small_height * static_cast<TestType>(ui_multiplier_point.GetY()));
            CHECK(res_size.GetDepth()  == small_depth  * static_cast<TestType>(ui_multiplier_point.GetZ()));
        }

        SECTION("Inplace division by point of integer type")
        {
            VolumeSize<TestType> res_size = big_size;
            res_size /= ui_multiplier_point;
            CHECK(res_size.GetWidth()  == big_width  / static_cast<TestType>(ui_multiplier_point.GetX()));
            CHECK(res_size.GetHeight() == big_height / static_cast<TestType>(ui_multiplier_point.GetY()));
            CHECK(res_size.GetDepth()  == big_depth  / static_cast<TestType>(ui_multiplier_point.GetZ()));
        }
    }
    else
    {
        const float fp_multiplier = 2.4F;

        SECTION("Multiplication by scalar of floating point type")
        {
            const VolumeSize<TestType> res_size = small_size * fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier)));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(small_depth)  * fp_multiplier)));
        }

        SECTION("Division by scalar of floating point type")
        {
            const VolumeSize<TestType> res_size = big_size / fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier)));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(big_depth)  / fp_multiplier)));
        }

        SECTION("Inplace multiplication by scalar of floating point type")
        {
            VolumeSize<TestType> res_size = small_size;
            res_size *= fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier)));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(small_depth)  * fp_multiplier)));
        }

        SECTION("Inplace division by scalar of floating point type")
        {
            VolumeSize<TestType> res_size = big_size;
            res_size /= fp_multiplier;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier)));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier)));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(big_depth)  / fp_multiplier)));
        }

        const VolumeSize<float> fp_multiplier_size(2.4F, 3.4F, 4.5F);

        SECTION("Multiplication by size of floating point type")
        {
            const VolumeSize<TestType> res_size = small_size * fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_size.GetHeight())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(small_depth)  * fp_multiplier_size.GetDepth())));
        }

        SECTION("Division by size of floating point type")
        {
            const VolumeSize<TestType> res_size = big_size / fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_size.GetHeight())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(big_depth)  / fp_multiplier_size.GetDepth())));
        }

        SECTION("Inplace multiplication by size of floating point type")
        {
            VolumeSize<TestType> res_size = small_size;
            res_size *= fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_size.GetHeight())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(small_depth)  * fp_multiplier_size.GetDepth())));
        }

        SECTION("Inplace division by size of floating point type")
        {
            VolumeSize<TestType> res_size = big_size;
            res_size /= fp_multiplier_size;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_size.GetWidth())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_size.GetHeight())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(big_depth)  / fp_multiplier_size.GetDepth())));
        }

        const Point3T<float> fp_multiplier_point(2.6F, 3.6F, 4.5F);

        SECTION("Multiplication by point of floating point type")
        {
            const VolumeSize<TestType> res_size = small_size * fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_point.GetY())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(small_depth)  * fp_multiplier_point.GetZ())));
        }

        SECTION("Division by point of floating point type")
        {
            const VolumeSize<TestType> res_size = big_size / fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_point.GetY())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(big_depth)  / fp_multiplier_point.GetZ())));
        }

        SECTION("Inplace multiplication by point of floating point type")
        {
            VolumeSize<TestType> res_size = small_size;
            res_size *= fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(small_width)  * fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(small_height) * fp_multiplier_point.GetY())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(small_depth)  * fp_multiplier_point.GetZ())));
        }

        SECTION("Inplace division by point of floating point type")
        {
            VolumeSize<TestType> res_size = big_size;
            res_size /= fp_multiplier_point;
            CHECK(res_size.GetWidth()  == static_cast<TestType>(std::round(static_cast<float>(big_width)  / fp_multiplier_point.GetX())));
            CHECK(res_size.GetHeight() == static_cast<TestType>(std::round(static_cast<float>(big_height) / fp_multiplier_point.GetY())));
            CHECK(res_size.GetDepth()  == static_cast<TestType>(std::round(static_cast<float>(big_depth)  / fp_multiplier_point.GetZ())));
        }
    }
}

TEMPLATE_TEST_CASE("Volume Size Conversion to Other Types", "[volume][size][convert]", VOLUME_SIZE_TYPES)
{
    const VolumeSize<TestType> test_size(TestType(1), TestType(2), TestType(3));

    if constexpr (std::is_floating_point_v<TestType>)
    {
        SECTION("Conversion to integer size")
        {
            CHECK(static_cast<VolumeSize<uint32_t>>(test_size) == VolumeSize<uint32_t>(1U, 2U, 3U));
        }
    }
    else
    {
        SECTION("Conversion to floating point size")
        {
            CHECK(static_cast<VolumeSize<float>>(test_size) == VolumeSize<float>(1.F, 2.F, 3.F));
        }
    }

    SECTION("Conversion to mutable rect size")
    {
        VolumeSize<TestType> mutable_size = test_size;
        mutable_size.AsRectSize().SetWidth(TestType(3));
        CHECK(mutable_size.AsRectSize() == RectSize<TestType>(TestType(3), TestType(2)));
    }

    SECTION("Conversion to const rect size")
    {
        CHECK(test_size.AsRectSize() == RectSize<TestType>(TestType(1), TestType(2)));
    }

    SECTION("Conversion to boolean")
    {
        CHECK_FALSE(static_cast<bool>(VolumeSize<TestType>()));
        CHECK_FALSE(static_cast<bool>(VolumeSize<TestType>(TestType(1), TestType(0), TestType(0))));
        CHECK(static_cast<bool>(test_size));
    }

    SECTION("Conversion to string")
    {
        CHECK(static_cast<std::string>(test_size) == "Sz(1 x 2 x 3)");
    }
}

TEMPLATE_TEST_CASE("Volume Size Property Accessors", "[volume][size][accessors]", VOLUME_SIZE_TYPES)
{
    const TestType test_width  = TestType(123) / 2;
    const TestType test_height = TestType(235) / 3;
    const TestType test_depth = TestType(345) / 4;
    const VolumeSize<TestType> test_size(test_width, test_height, test_depth);

    SECTION("Depth accessors")
    {
        VolumeSize<TestType> vol_size;
        CHECK_NOTHROW(vol_size.SetDepth(345));
        CHECK(vol_size.GetDepth() == 345);
        CHECK(vol_size == VolumeSize<TestType>(0, 0, 345));

        if constexpr (std::is_signed_v<TestType>)
        {
            CHECK_THROWS_AS(vol_size.SetDepth(-345), Methane::ArgumentExceptionBase<std::out_of_range>);
        }
    }

    SECTION("Get pixels count")
    {
        CHECK(test_size.GetPixelsCount() == test_width * test_height * test_depth);
    }

    SECTION("Get longest side")
    {
        CHECK(test_size.GetLongestSide() == std::max({ test_width, test_height, test_depth }));
    }
}