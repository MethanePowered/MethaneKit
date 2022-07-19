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

FILE: Tests/Graphics/Types/VolumeTest.cpp
Unit-tests of the Volume data type

******************************************************************************/

#include <Methane/Graphics/Volume.hpp>
#include <Methane/Graphics/TypeFormatters.hpp>

#include <catch2/catch_template_test_macros.hpp>

using namespace Methane::Graphics;

template<typename D> using VolumePtInt    = Volume<int32_t, D>;
template<typename D> using VolumePtUint   = Volume<uint32_t, D>;
template<typename D> using VolumePtFloat  = Volume<float, D>;
template<typename D> using VolumePtDouble = Volume<double, D>;

#define RECT_TYPES_PRODUCT \
    (VolumePtInt, VolumePtUint, VolumePtFloat, VolumePtDouble), \
    (int32_t,     uint32_t,     float,         double)

TEMPLATE_PRODUCT_TEST_CASE("Volume Initialization", "[volume][init]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;

    const PointType test_origin(12, 34, 56);
    const SizeType  test_size(671, 782, 893);

    SECTION("Default initialization of empty volume")
    {
        const TestType test_vol;
        CHECK(test_vol.origin == PointType());
        CHECK(test_vol.size == SizeType());
    }

    SECTION("Origin only initialization")
    {
        const TestType test_vol(test_origin);
        CHECK(test_vol.origin == test_origin);
        CHECK(test_vol.size == SizeType());
    }

    SECTION("Size only initialization")
    {
        const TestType test_vol(test_size);
        CHECK(test_vol.origin == PointType());
        CHECK(test_vol.size == test_size);
    }

    SECTION("Origin and size initialization")
    {
        const TestType test_vol(test_origin, test_size);
        CHECK(test_vol.origin == test_origin);
        CHECK(test_vol.size == test_size);
    }

    SECTION("Coordinates and dimensions initialization")
    {
        const TestType test_vol(12, 34, 56, 671, 782, 893);
        CHECK(test_vol.origin == test_origin);
        CHECK(test_vol.size == test_size);
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Volumes Comparison", "[volume][compare]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;

    const PointType test_origin(12, 34, 56);
    const SizeType  test_size(671, 782, 893);
    const TestType  test_vol(test_origin, test_size);

    SECTION("Equality")
    {
        CHECK(test_vol == TestType(test_origin, test_size));
        CHECK_FALSE(test_vol == TestType(test_origin));
        CHECK_FALSE(test_vol == TestType(test_size));
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(test_vol != TestType(test_origin, test_size));
        CHECK(test_vol != TestType(test_origin));
        CHECK(test_vol != TestType(test_size));
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Volume Math Operations", "[volume][math]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;
    using CoordType = typename PointType::CoordinateType;
    using DimType   = typename SizeType::DimensionType;

    const PointType test_origin(2, 4, 6);
    const SizeType  test_size(6, 8, 10);
    const TestType  test_vol(test_origin, test_size);

    SECTION("Multiplication by scalar of coordinate type")
    {
        const TestType res_volume = test_vol * CoordType(2);
        CHECK(res_volume.origin == PointType(4, 8, 12));
        CHECK(res_volume.size   == SizeType(12, 16, 20));
    }

    SECTION("Multiplication by scalar of dimension type")
    {
        const TestType res_volume = test_vol * DimType(2);
        CHECK(res_volume.origin == PointType(4, 8, 12));
        CHECK(res_volume.size   == SizeType(12, 16, 20));
    }

    SECTION("Division by scalar of coordinate type")
    {
        const TestType res_volume = test_vol / CoordType(2);
        CHECK(res_volume.origin == PointType(1, 2, 3));
        CHECK(res_volume.size   == SizeType(3, 4, 5));
    }

    SECTION("Division by scalar of dimension type")
    {
        const TestType res_volume = test_vol / DimType(2);
        CHECK(res_volume.origin == PointType(1, 2, 3));
        CHECK(res_volume.size   == SizeType(3, 4, 5));
    }

    SECTION("Inplace multiplication by scalar of coordinate type")
    {
        TestType res_volume = test_vol;
        res_volume *= CoordType(2);
        CHECK(res_volume.origin == PointType(4, 8, 12));
        CHECK(res_volume.size   == SizeType(12, 16, 20));
    }

    SECTION("Inplace multiplication by scalar of dimension type")
    {
        TestType res_volume = test_vol;
        res_volume *= DimType(2);
        CHECK(res_volume.origin == PointType(4, 8, 12));
        CHECK(res_volume.size   == SizeType(12, 16, 20));
    }

    SECTION("Inplace division by scalar of coordinate type")
    {
        TestType res_volume = test_vol;
        res_volume /= CoordType(2);
        CHECK(res_volume.origin == PointType(1, 2, 3));
        CHECK(res_volume.size   == SizeType(3, 4, 5));
    }

    SECTION("Inplace division by scalar of dimension type")
    {
        TestType res_volume = test_vol;
        res_volume /= DimType(2);
        CHECK(res_volume.origin == PointType(1, 2, 3));
        CHECK(res_volume.size   == SizeType(3, 4, 5));
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Volume Conversion to Other Types", "[volume][convert]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;
    using CoordType = typename PointType::CoordinateType;
    using DimType   = typename SizeType::DimensionType;

    const PointType test_origin(12, 34, 56);
    const SizeType  test_size(671, 782, 893);
    const TestType  test_vol(test_origin, test_size);

    if constexpr (std::is_floating_point_v<CoordType>)
    {
        if constexpr (std::is_floating_point_v<DimType>)
        {
            SECTION("Convert to integer point and integer size volume")
            {
                CHECK(static_cast<Volume<int32_t, uint32_t>>(test_vol) == Volume<int32_t, uint32_t>(12, 34, 56, 671U, 782U, 893U));
            }
        }
        else
        {
            SECTION("Convert to integer point and floating size volume")
            {
                CHECK(static_cast<Volume<int32_t, float>>(test_vol) == Volume<int32_t, float>(12, 34, 56, 671.F, 782.F, 893.F));
            }
        }
    }
    else
    {
        if constexpr (std::is_floating_point_v<DimType>)
        {
            SECTION("Convert to floating point and integer size volume")
            {
                CHECK(static_cast<Volume<float, uint32_t>>(test_vol) == Volume<float, uint32_t>(12.F, 34.F, 56.F, 671U, 782U, 893U));
            }
        }
        else
        {
            SECTION("Convert to floating point and floating size volume")
            {
                CHECK(static_cast<Volume<float, float>>(test_vol) == Volume<float, float>(12, 34, 56, 671.F, 782.F, 893.F));
            }
        }
    }

    SECTION("Conversion to string")
    {
        CHECK(static_cast<std::string>(test_vol) == "Vol[P(12, 34, 56) : Sz(671 x 782 x 893)]");
    }
}

TEMPLATE_PRODUCT_TEST_CASE("Volume Property Getters", "[volume][accessors]", RECT_TYPES_PRODUCT)
{
    using PointType = typename TestType::Point;
    using SizeType  = typename TestType::Size;
    using CoordType = typename PointType::CoordinateType;

    const PointType test_origin(12, 34, 56);
    const SizeType  test_size(671, 782, 893);
    const TestType  test_vol(test_origin, test_size);

    SECTION("Left coordinate getter")
    {
        CHECK(test_vol.GetLeft() == CoordType(12));
    }

    SECTION("Right coordinate getter")
    {
        CHECK(test_vol.GetRight() == CoordType(683));
    }

    SECTION("Top coordinate getter")
    {
        CHECK(test_vol.GetTop() == CoordType(34));
    }

    SECTION("Bottom coordinate getter")
    {
        CHECK(test_vol.GetBottom() == CoordType(816));
    }

    SECTION("Near coordinate getter")
    {
        CHECK(test_vol.GetNear() == CoordType(56));
    }

    SECTION("Far coordinate getter")
    {
        CHECK(test_vol.GetFar() == CoordType(949));
    }
}