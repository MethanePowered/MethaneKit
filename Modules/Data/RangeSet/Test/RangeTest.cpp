/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Test/RangeTest.cpp
Unit tests of the Range data type

******************************************************************************/

#include <catch2/catch.hpp>

#include <Methane/Data/Range.hpp>

using namespace Methane::Data;

TEST_CASE("Range initialization", "[range]")
{
    SECTION("Constructor arguments")
    {
        const Range<int32_t> int_range(123, 456);
        CHECK(int_range.GetStart() == 123);
        CHECK(int_range.GetEnd() == 456);
    }

    SECTION("Initializer list")
    {
        const Range<float> float_range = { 1.23f, 4.56f };
        CHECK(float_range.GetStart() == 1.23f);
        CHECK(float_range.GetEnd() == 4.56f);
    }

    SECTION("Copy constructor")
    {
        const Range<char> char_range_a('a', 'c');
        const Range<char> char_range_b(char_range_a);
        CHECK(char_range_b.GetStart() == char_range_a.GetStart());
        CHECK(char_range_b.GetEnd() == char_range_a.GetEnd());
    }

    SECTION("Growth order only allowed")
    {
        CHECK_THROWS_AS(Range<uint32_t>(5, 1), std::invalid_argument);
        CHECK_THROWS_AS(Range<double>(4.56, 1.23), std::invalid_argument);
    }
}

TEST_CASE("Range length", "[range]")
{
    CHECK(Range<int32_t>(0, 0).GetLength() == 0);
    CHECK(Range<int32_t>(0, 0).IsEmpty());

    CHECK(Range<int32_t>(0, 1).GetLength() == 1);
    CHECK_FALSE(Range<int32_t>(0, 1).IsEmpty());

    CHECK(Range<float>(1.5f, 3.6f).GetLength() == 2.1f);
    CHECK(Range<float>(3.3f, 6.6f).GetLength() == 3.3f);
}

TEST_CASE("Range relations", "[range]")
{
    const Range<uint32_t> range_a(0, 4);
    const Range<uint32_t> range_b(4, 6);
    const Range<uint32_t> range_c(2, 5);
    const Range<uint32_t> range_d(5, 8);
    const Range<uint32_t> range_e(2, 9);

    SECTION("Equal")
    {
        CHECK(range_a == Range<uint32_t>(range_a));
        CHECK_FALSE(range_a == range_b);
        CHECK(range_a != range_b);
    }

    SECTION("Less")
    {
        CHECK(range_a < range_b);
        CHECK_FALSE(range_c < range_a);
    }

    SECTION("Adjacent")
    {
        CHECK(range_a.IsAdjacent(range_b));
        CHECK(range_b.IsAdjacent(range_a));

        CHECK_FALSE(range_a.IsAdjacent(range_c));
        CHECK_FALSE(range_c.IsAdjacent(range_a));
    }

    SECTION("Overlapping")
    {
        CHECK(range_a.IsOverlapping(range_c));
        CHECK(range_c.IsOverlapping(range_a));

        CHECK_FALSE(range_a.IsOverlapping(range_b));
        CHECK_FALSE(range_b.IsOverlapping(range_a));

        CHECK_FALSE(range_a.IsOverlapping(range_d));
        CHECK_FALSE(range_d.IsOverlapping(range_a));
    }

    SECTION("Mergeable")
    {
        CHECK(range_a.IsMergeable(range_c));
        CHECK(range_c.IsMergeable(range_a));

        CHECK(range_a.IsMergeable(range_b));
        CHECK(range_b.IsMergeable(range_a));

        CHECK_FALSE(range_a.IsMergeable(range_d));
        CHECK_FALSE(range_d.IsMergeable(range_a));
    }

    SECTION("Contained")
    {
        CHECK(range_e.Contains(range_b));
        CHECK_FALSE(range_b.Contains(range_e));

        CHECK(range_e.Contains(range_c));
        CHECK_FALSE(range_c.Contains(range_e));

        CHECK_FALSE(range_a.Contains(range_e));
        CHECK_FALSE(range_e.Contains(range_a));
    }
}

TEST_CASE("Range operations", "[range]")
{
    const Range<uint32_t> range_a(0, 4);
    const Range<uint32_t> range_b(4, 6);
    const Range<uint32_t> range_c(2, 5);
    const Range<uint32_t> range_d(5, 8);
    const Range<uint32_t> range_e(2, 9);

    SECTION("Merge")
    {
        const Range<uint32_t> a_plus_b_range(0, 6);
        CHECK(range_a + range_b == a_plus_b_range);
        CHECK(range_b + range_a == a_plus_b_range);

        const Range<uint32_t> a_plus_c_range(0, 5);
        CHECK(range_a + range_c == a_plus_c_range);
        CHECK(range_c + range_a == a_plus_c_range);

        CHECK_THROWS_AS(range_a + range_d, std::invalid_argument);
    }

    SECTION("Intersection")
    {
        const Range<uint32_t> a_inter_b_range(4, 4);
        CHECK(range_a % range_b == a_inter_b_range);
        CHECK(range_b % range_a == a_inter_b_range);

        const Range<uint32_t> a_inter_c_range(2, 4);
        CHECK(range_a % range_c == a_inter_c_range);
        CHECK(range_c % range_a == a_inter_c_range);

        CHECK_THROWS_AS(range_a % range_d, std::invalid_argument);
    }

    SECTION("Subtraction")
    {
        CHECK(range_a - range_c == Range<uint32_t>(0, 2));
        CHECK(range_c - range_a == Range<uint32_t>(4, 5));

        CHECK(range_b - range_d == Range<uint32_t>(4, 5));
        CHECK(range_d - range_b == Range<uint32_t>(6, 8));

        CHECK_THROWS_AS(range_a - range_d, std::invalid_argument);
        CHECK_THROWS_AS(range_b - range_e, std::invalid_argument);
        CHECK_THROWS_AS(range_e - range_c, std::invalid_argument);
    }
}
