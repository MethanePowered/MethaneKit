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
    CHECK(point.GetX() == components[0]);
    CHECK(point.GetY() == components[1]);
    if constexpr (size > 2)
        CHECK(point.GetZ() == components[2]);
    if constexpr (size > 3)
        CHECK(point.GetW() == components[3]);
}

TEMPLATE_TEST_CASE_SIG("Point Initialization", "[point][init]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> coords_arr = CreateComponents<T, size>();

    SECTION("Default initialization with zeros")
    {
        CheckPoint(Point<T, size>(), CreateComponents<T, size>(T(0), T(0)));
    }

    SECTION("Initialization with component values")
    {
        if constexpr (size == 2)
            CheckPoint(Point<T, 2>(coords_arr[0], coords_arr[1]), coords_arr);
        if constexpr (size == 3)
            CheckPoint(Point<T, 3>(coords_arr[0], coords_arr[1], coords_arr[2]), coords_arr);
        if constexpr (size == 4)
            CheckPoint(Point<T, 4>(coords_arr[0], coords_arr[1], coords_arr[2], coords_arr[3]), coords_arr);
    }

    SECTION("Initialization with array")
    {
        CheckPoint(Point<T, size>(coords_arr), coords_arr);
    }

    SECTION("Initialization with moved array")
    {
        CheckPoint(Point<T, size>(CreateComponents<T, size>()), coords_arr);
    }

    SECTION("Initialization with HLSL vector reference")
    {
        const HlslVector<T, size> hlsl_vec = CreateHlslVector(coords_arr);
        CheckPoint(Point<T, size>(hlsl_vec), coords_arr);
    }

    SECTION("Initialization with moved HLSL vector")
    {
        CheckPoint(Point<T, size>(CreateHlslVector(coords_arr)), coords_arr);
    }

    SECTION("Copy initialization from the same point type")
    {
        const Point<T, size> point(coords_arr);
        CheckPoint(Point<T, size>(point), coords_arr);
    }

    SECTION("Move initialization from the same point type")
    {
        Point<T, size> point(coords_arr);
        CheckPoint(Point<T, size>(std::move(point)), coords_arr);
    }

    SECTION("Copy assignment initialization")
    {
        const Point<T, size> point(coords_arr);
        Point<T, size> copy_point;
        copy_point = point;
        CheckPoint(copy_point, coords_arr);
    }

    SECTION("Move assignment initialization")
    {
        Point<T, size> point(coords_arr);
        Point<T, size> copy_point;
        copy_point = std::move(point);
        CheckPoint(copy_point, coords_arr);
    }

    if constexpr (!std::is_same_v<T, int32_t>)
    {
        SECTION("Copy initialization from integer point")
        {
            CheckPoint(static_cast<Point<int32_t, size>>(Point<T, size>(coords_arr)), CreateComponents<int32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, uint32_t>)
    {
        SECTION("Copy initialization from unsigned integer point")
        {
            CheckPoint(static_cast<Point<uint32_t, size>>(Point<T, size>(coords_arr)), CreateComponents<uint32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, float>)
    {
        SECTION("Copy initialization from float point")
        {
            CheckPoint(static_cast<Point<float, size>>(Point<T, size>(coords_arr)), CreateComponents<float, size>());
        }
    }
    if constexpr (!std::is_same_v<T, double>)
    {
        SECTION("Copy initialization from double point")
        {
            CheckPoint(static_cast<Point<double, size>>(Point<T, size>(coords_arr)), CreateComponents<double, size>());
        }
    }
}
