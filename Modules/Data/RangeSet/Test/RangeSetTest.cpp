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

FILE: Test/RangeSetTest.cpp
Unit tests of the RangeSet data type

******************************************************************************/

#include <catch2/catch.hpp>

#include <Methane/Data/RangeSet.hpp>

using namespace Methane::Data;

TEST_CASE("Range set initialization", "[range-set]")
{
    SECTION("Default constructor")
    {
        const RangeSet<uint32_t> range_set;
        CHECK(range_set.IsEmpty());
    }

    SECTION("Initializer list with non-intersecting ranges")
    {
        const RangeSet<uint32_t> range_set = { { 0, 2 }, { 4, 8 }, { 11, 12 } };
        CHECK(range_set.Size() == 3);
    }
        
    SECTION("Initializer list with intersecting ranges")
    {
        const RangeSet<uint32_t> range_set = { { 0, 5 }, { 4, 8 }, { 11, 12 } };
        CHECK(range_set.Size() == 2);
    }
    
    SECTION("Copy constructor")
    {
        const RangeSet<uint32_t> orig_range_set = { { 0, 5 }, { 4, 8 }, { 11, 12 } };
        const RangeSet<uint32_t> copy_range_set(orig_range_set);
        CHECK(copy_range_set == orig_range_set);
    }
}

TEST_CASE("Range set add", "[range-set]")
{
    const RangeSet<uint32_t> test_range_set = {
        { 0, 2 }, { 4, 8 }, { 11, 12 }, { 17, 20 }, { 25, 29 }
    };
    
    SECTION("Adding non-mergeable range")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Add({ 14, 16 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 4, 8 }, { 11, 12 }, { 14, 16 }, { 17, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }
    
    SECTION("Adding mergeable range in the middle")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Add({ 5, 12 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 4, 12 }, { 17, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }

    SECTION("Adding mergeable range in the beginning")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Add({ 0, 7 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 8 }, { 11, 12 }, { 17, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }

    SECTION("Adding mergeable range in the end")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Add({ 26, 35 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 4, 8 }, { 11, 12 }, { 17, 20 }, { 25, 35 } };
        CHECK(range_set == reference_set);
    }

    SECTION("Adding adjacent range in the middle")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Add({ 8, 11 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 4, 12 }, { 17, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }
}

TEST_CASE("Range set remove", "[range-set]")
{
    const RangeSet<uint32_t> test_range_set = {
        { 0, 2 }, { 4, 8 }, { 11, 12 }, { 17, 20 }, { 25, 29 }
    };

    SECTION("Remove adjacent range")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Remove({ 8, 11 });

        CHECK(range_set == test_range_set);
    }

    SECTION("Remove existing full range")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Remove({ 4, 8 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 11, 12 }, { 17, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }

    SECTION("Remove overlapping range from middle")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Remove({ 6, 18 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 4, 6 }, { 18, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }

    SECTION("Remove overlapping range from beginning")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Remove({ 0, 3 });

        const std::set<Range<uint32_t>> reference_set = { { 4, 8 }, { 11, 12 }, { 17, 20 }, { 25, 29 } };
        CHECK(range_set == reference_set);
    }

    SECTION("Remove overlapping range from end")
    {
        RangeSet<uint32_t> range_set(test_range_set);
        range_set.Remove({ 23, 30 });

        const std::set<Range<uint32_t>> reference_set = { { 0, 2 }, { 4, 8 }, { 11, 12 }, { 17, 20 } };
        CHECK(range_set == reference_set);
    }

}