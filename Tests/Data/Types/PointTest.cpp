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

FILE: Tests/Data/Types/PointTest.cpp
Unit tests of the Point data type wrapping HLSL++ vector

******************************************************************************/

#include "TestHelpers.hpp"

#include <Methane/Data/Point.hpp>

#include <catch2/catch.hpp>
#include <sstream>

using namespace Methane::Data;

template<typename T, size_t size, typename = std::enable_if_t<2 <= size && size <= 4>>
void CheckPoint(const Point<T, size>& point, const std::array<T, size>& components)
{
    CHECK(point.GetX() == Approx(components[0]));
    CHECK(point.GetY() == Approx(components[1]));
    if constexpr (size > 2)
        CHECK(point.GetZ() == Approx(components[2]));
    if constexpr (size > 3)
        CHECK(point.GetW() == Approx(components[3]));
}

TEMPLATE_TEST_CASE_SIG("Point Initialization", "[point][init]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> test_arr = CreateComponents<T, size>();

    SECTION("Default initialization with zeros")
    {
        CheckPoint(Point<T, size>(), CreateComponents<T, size>(T(0), T(0)));
    }

    SECTION("Initialization with component values")
    {
        if constexpr (size == 2)
            CheckPoint(Point<T, 2>(test_arr[0], test_arr[1]), test_arr);
        if constexpr (size == 3)
            CheckPoint(Point<T, 3>(test_arr[0], test_arr[1], test_arr[2]), test_arr);
        if constexpr (size == 4)
            CheckPoint(Point<T, 4>(test_arr[0], test_arr[1], test_arr[2], test_arr[3]), test_arr);
    }

    SECTION("Initialization with array")
    {
        CheckPoint(Point<T, size>(test_arr), test_arr);
    }

    SECTION("Initialization with moved array")
    {
        CheckPoint(Point<T, size>(CreateComponents<T, size>()), test_arr);
    }

    SECTION("Initialization with HLSL vector reference")
    {
        const HlslVector<T, size> hlsl_vec = CreateHlslVector(test_arr);
        CheckPoint(Point<T, size>(hlsl_vec), test_arr);
    }

    SECTION("Initialization with moved HLSL vector")
    {
        CheckPoint(Point<T, size>(CreateHlslVector(test_arr)), test_arr);
    }

    SECTION("Copy initialization from the same point type")
    {
        const Point<T, size> point(test_arr);
        CheckPoint(Point<T, size>(point), test_arr);
    }

    SECTION("Move initialization from the same point type")
    {
        Point<T, size> point(test_arr);
        CheckPoint(Point<T, size>(std::move(point)), test_arr);
    }

    SECTION("Copy assignment initialization")
    {
        const Point<T, size> point(test_arr);
        Point<T, size> copy_point;
        copy_point = point;
        CheckPoint(copy_point, test_arr);
    }

    SECTION("Move assignment initialization")
    {
        Point<T, size> point(test_arr);
        Point<T, size> copy_point;
        copy_point = std::move(point);
        CheckPoint(copy_point, test_arr);
    }

    if constexpr (!std::is_same_v<T, int32_t>)
    {
        SECTION("Copy initialization from integer point")
        {
            CheckPoint(static_cast<Point<int32_t, size>>(Point<T, size>(test_arr)), CreateComponents<int32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, uint32_t>)
    {
        SECTION("Copy initialization from unsigned integer point")
        {
            CheckPoint(static_cast<Point<uint32_t, size>>(Point<T, size>(test_arr)), CreateComponents<uint32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, float>)
    {
        SECTION("Copy initialization from float point")
        {
            CheckPoint(static_cast<Point<float, size>>(Point<T, size>(test_arr)), CreateComponents<float, size>());
        }
    }
    if constexpr (!std::is_same_v<T, double>)
    {
        SECTION("Copy initialization from double point")
        {
            CheckPoint(static_cast<Point<double, size>>(Point<T, size>(test_arr)), CreateComponents<double, size>());
        }
    }
}

TEMPLATE_TEST_CASE_SIG("Point Conversions to Other Types", "[point][convert]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> test_arr = CreateComponents<T, size>();
    const Point<T, size>      test_point(test_arr);

    if constexpr (!std::is_same_v<T, int32_t>)
    {
        SECTION("Cast to integer point")
        {
            CheckPoint(static_cast<Point<int32_t, size>>(test_point), CreateComponents<int32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, uint32_t>)
    {
        SECTION("Cast to unsigned integer point")
        {
            CheckPoint(static_cast<Point<uint32_t, size>>(test_point), CreateComponents<uint32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, float>)
    {
        SECTION("Cast to float point")
        {
            CheckPoint(static_cast<Point<float, size>>(test_point), CreateComponents<float, size>());
        }
    }
    if constexpr (!std::is_same_v<T, double>)
    {
        SECTION("Cast to double point")
        {
            CheckPoint(static_cast<Point<double, size>>(test_point), CreateComponents<double, size>());
        }
    }

    SECTION("Cast to string")
    {
        std::stringstream ss;
        ss << "P(" << test_arr[0];
        for(size_t i = 1; i < size; ++i)
            ss << ", " << test_arr[i];
        ss << ")";
        CHECK(static_cast<std::string>(test_point) == ss.str());
    }

    SECTION("Cast to HLSL vector")
    {
        CHECK(hlslpp::all(static_cast<HlslVector<T, size>>(test_point) == CreateHlslVector(test_arr)));
    }

    SECTION("Convert to HLSL vector")
    {
        CHECK(hlslpp::all(test_point.AsHlsl() == CreateHlslVector(test_arr)));
    }
}

TEMPLATE_TEST_CASE_SIG("Point Coordinate Accessors and Property Getters", "[point][accessors]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> test_arr = CreateComponents<T, size>();
    const Point<T, size>      test_point(test_arr);
    const T                   new_value(123);

    SECTION("X-coordinate getter and setter")
    {
        CHECK(test_point.GetX() == Approx(test_arr[0]));
        auto new_arr = test_arr; new_arr[0] = new_value;
        CheckPoint(Point<T, size>(test_arr).SetX(new_value), new_arr);
    }

    SECTION("Y-coordinate getter and setter")
    {
        CHECK(test_point.GetY() == Approx(test_arr[1]));
        auto new_arr = test_arr; new_arr[1] = new_value;
        CheckPoint(Point<T, size>(test_arr).SetY(new_value), new_arr);
    }

    if constexpr (size > 2)
    {
        SECTION("Z-coordinate getter and setter")
        {
            CHECK(test_point.GetZ() == Approx(test_arr[2]));
            auto new_arr = test_arr; new_arr[2] = new_value;
            CheckPoint(Point<T, size>(test_arr).SetZ(new_value), new_arr);
        }
    }

    if constexpr (size > 3)
    {
        SECTION("W-coordinate getter and setter")
        {
            CHECK(test_point.GetW() == Approx(test_arr[3]));
            auto new_arr = test_arr; new_arr[3] = new_value;
            CheckPoint(Point<T, size>(test_arr).SetW(new_value), new_arr);
        }
    }

    SECTION("Length getter")
    {
        T length = 0;
        for(T component : test_arr)
            length += component * component;
        length = static_cast<T>(std::sqrt(length));
        CHECK(test_point.GetLength() == Approx(length));
    }

    SECTION("Length squared getter")
    {
        T sq_length = 0;
        for(T component : test_arr)
            sq_length += component * component;
        CHECK(test_point.GetLengthSquared() == Approx(sq_length));
    }
}

TEMPLATE_TEST_CASE_SIG("Points Comparison", "[point][compare]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> test_arr = CreateComponents<T, size>(T(1), T(1));
    const Point<T, size>  test_point(test_arr);
    const Point<T, size>  identity_point(CreateEqualComponents<T, size>(T(1)));

    SECTION("Points equality comparison")
    {
        CHECK(Point<T, size>(test_arr) == Point<T, size>(test_arr));
        CHECK_FALSE(Point<T, size>(test_arr) == Point<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }

    SECTION("Points non-equality comparison")
    {
        CHECK_FALSE(Point<T, size>(test_arr) != Point<T, size>(test_arr));
        CHECK(Point<T, size>(test_arr) != Point<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }
}

TEMPLATE_TEST_CASE_SIG("Points Math Operations", "[point][math]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> test_arr = CreateComponents<T, size>(T(1), T(1));
    const Point<T, size>  test_point(test_arr);
    const Point<T, size>  identity_point(CreateEqualComponents<T, size>(T(1)));

    SECTION("Addition")
    {
        CheckPoint(test_point + identity_point, CreateComponents<T, size>(T(2), T(1)));
    }

    SECTION("Inplace addition")
    {
        Point<T, size> point(test_point);
        point += identity_point;
        CheckPoint(point, CreateComponents<T, size>(T(2), T(1)));
    }

    SECTION("Subtraction")
    {
        CheckPoint(test_point - identity_point, CreateComponents<T, size>(T(0), T(1)));
    }

    SECTION("Inplace subtraction")
    {
        Point<T, size> point(test_point);
        point -= identity_point;
        CheckPoint(point, CreateComponents<T, size>(T(0), T(1)));
    }

    SECTION("Multiplication by scalar")
    {
        CheckPoint(test_point * T(2), CreateComponents<T, size>(T(2), T(2)));
    }

    SECTION("Inplace multiplication by scalar")
    {
        Point<T, size> point(test_point);
        point *= T(2);
        CheckPoint(point, CreateComponents<T, size>(T(2), T(2)));
    }

    SECTION("Division by scalar")
    {
        CheckPoint(Point<T, size>(CreateComponents<T, size>(T(2), T(2))) / T(2), test_arr);
    }

    if constexpr (!std::is_integral_v<T>)
    {
        SECTION("Inplace division by scalar")
        {
            Point<T, size> point(CreateComponents<T, size>(T(2), T(2)));
            point /= T(2);
            CheckPoint(point, test_arr);
        }
    }
}