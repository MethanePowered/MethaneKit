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

FILE: Tests/Data/Types/RawVectorTest.cpp
Unit-tests of the RawVector data type

******************************************************************************/

#include "TestHelpers.hpp"

#include <Methane/Data/Vector.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>
#include <array>

#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(X,Y) 0
#endif

using namespace Methane::Data;
using Catch::Approx;

template<typename T, size_t size>
struct Catch::StringMaker<RawVector<T, size>>
{
    static std::string convert(const RawVector<T, size>& v)
    {
        return static_cast<std::string>(v);
    }
};

template<typename T, size_t size, typename = std::enable_if_t<2 <= size && size <= 4>>
void CheckRawVector(const RawVector<T, size>& vec, const std::array<T, size>& components)
{
    CHECK(vec[0] == Approx(components[0]));
    CHECK(vec[1] == Approx(components[1]));
    if constexpr (size > 2)
        CHECK(vec[2] == Approx(components[2]));
    if constexpr (size > 3)
        CHECK(vec[3] == Approx(components[3]));
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Initialization", "[vector][init]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>();

    SECTION("Vector size equals sum of its component sizes")
    {
        CHECK(sizeof(RawVector<T, size>) == sizeof(T) * size);
    }

    SECTION("Default initialization with zeros")
    {
        CheckRawVector(RawVector<T, size>(), CreateComponents<T, size>(T(0), T(0)));
    }

    SECTION("Initialization with component values")
    {
        if constexpr (size == 2)
            CheckRawVector(RawVector<T, 2>(raw_arr[0], raw_arr[1]), raw_arr);
        if constexpr (size == 3)
            CheckRawVector(RawVector<T, 3>(raw_arr[0], raw_arr[1], raw_arr[2]), raw_arr);
        if constexpr (size == 4)
            CheckRawVector(RawVector<T, 4>(raw_arr[0], raw_arr[1], raw_arr[2], raw_arr[3]), raw_arr);
    }

    SECTION("Initialization with array")
    {
        CheckRawVector(RawVector<T, size>(raw_arr), raw_arr);
    }

    SECTION("Initialization with array pointer")
    {
        CheckRawVector(RawVector<T, size>(raw_arr.data()), raw_arr);
    }

    SECTION("Initialization with moved array")
    {
        CheckRawVector(RawVector<T, size>(CreateComponents<T, size>()), CreateComponents<T, size>());
    }

    SECTION("Initialization with HLSL vector")
    {
        const HlslVector<T, size> hlsl_vec = CreateHlslVector(raw_arr);
        CheckRawVector(RawVector<T, size>(hlsl_vec), raw_arr);
    }

    SECTION("Copy initialization from the same vector type")
    {
        const RawVector<T, size> vec(raw_arr);
        CheckRawVector(RawVector<T, size>(vec), raw_arr);
    }

    SECTION("Move initialization from the same vector type")
    {
        RawVector<T, size> vec(raw_arr);
        CheckRawVector(RawVector<T, size>(std::move(vec)), raw_arr);
    }

    SECTION("Copy assignment initialization")
    {
        const RawVector<T, size> raw_vec(raw_arr);
        RawVector<T, size> copy_vec;
        copy_vec = raw_vec;
        CheckRawVector(copy_vec, raw_arr);
    }

    SECTION("Move assignment initialization")
    {
        RawVector<T, size> raw_vec(raw_arr);
        RawVector<T, size> copy_vec;
        copy_vec = std::move(raw_vec);
        CheckRawVector(copy_vec, raw_arr);
    }

    if constexpr (size > 2)
    {
        SECTION("Copy initialization from smaller vector size")
        {
            const std::array<T, size - 1> small_arr = CreateComponents<T, size - 1>();
            RawVector<T, size - 1>        small_vec(small_arr);
            CheckRawVector(RawVector<T, size>(small_vec, raw_arr.back()), raw_arr);
        }
    }
    if constexpr (size > 3)
    {
        SECTION("Copy initialization from much smaller vector size")
        {
            const std::array<T, size - 2> small_arr = CreateComponents<T, size - 2>();
            RawVector<T, size - 2>        small_vec(small_arr);
            CheckRawVector(RawVector<T, size>(small_vec, raw_arr[2], raw_arr[3]), raw_arr);
        }
    }
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Conversions to Other Types", "[vector][convert]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>();
    const RawVector<T, size>  raw_vec(raw_arr);

    if constexpr (!std::is_same_v<T, int32_t>)
    {
        SECTION("Cast to vector of integers")
        {
            CheckRawVector(static_cast<RawVector<int32_t, size>>(raw_vec), CreateComponents<int32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, uint32_t>)
    {
        SECTION("Cast to vector of unsigned integers")
        {
            CheckRawVector(static_cast<RawVector<uint32_t, size>>(raw_vec), CreateComponents<uint32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, float>)
    {
        SECTION("Cast to vector of floats")
        {
            CheckRawVector(static_cast<RawVector<float, size>>(raw_vec), CreateComponents<float, size>());
        }
    }
    if constexpr (!std::is_same_v<T, double>)
    {
        SECTION("Cast to vector of doubles")
        {
            CheckRawVector(static_cast<RawVector<double, size>>(raw_vec), CreateComponents<double, size>());
        }
    }

// FIXME: Workaround for compiler bug, reproducible with GCC v13.2.0 on Ubuntu 24.04
//        during RTL pass: fwprop1
//        Tests/Data/Types/RawVectorTest.cpp: In function ‘CATCH2_INTERNAL_TEMPLATE_TEST_16’:
//        Tests/Data/Types/RawVectorTest.cpp:202:1: internal compiler error: in simplify_const_unary_operation, at simplify-rtx.cc:2204
#if !__GNUC_PREREQ(13,0)
    SECTION("Cast to string")
    {
        std::stringstream ss;
        ss << "V(" << raw_arr[0];
        for(size_t i = 1; i < size; ++i)
            ss << ", " << raw_arr[i];
        ss << ")";
        CHECK(static_cast<std::string>(raw_vec) == ss.str());
    }

    SECTION("Convert to HLSL vector")
    {
        const HlslVector<T, size> hlsl_vec = raw_vec.AsHlsl();
        CHECK(hlslpp::all(hlsl_vec == CreateHlslVector(raw_arr)));
    }
#endif // !__GNUC_PREREQ(13,0)
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Component Accessors and Property Getters", "[vector][accessors]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>();
    const RawVector<T, size>  raw_vec(raw_arr);
    const T                   new_value(123);

    SECTION("Unsafe component getters by index")
    {
        for(size_t i = 0; i < size; ++i)
        {
            CHECK(raw_vec[i] == Approx(raw_arr[i]));
        }
    }

    SECTION("Unsafe component setters by index")
    {
        RawVector<T, size> raw_vec_mutable(raw_arr);
        const std::array<T, size> other_arr = CreateComponents<T, size>(T(5), T(2));
        for(size_t i = 0; i < size; ++i)
        {
            raw_vec_mutable[i] = other_arr[i];
        }
        CheckRawVector(raw_vec_mutable, other_arr);
    }

    SECTION("Safe component getters by index")
    {
        for(size_t i = 0; i < size; ++i)
        {
            CHECK(raw_vec.Get(i) == Approx(raw_arr[i]));
        }
        CHECK_THROWS_AS(raw_vec.Get(size + 1), Methane::ArgumentExceptionBase<std::out_of_range>);
    }

    SECTION("Safe component setters by index")
    {
        RawVector<T, size> raw_vec_mutable(raw_arr);
        const std::array<T, size> other_arr = CreateComponents<T, size>(T(5), T(2));
        for(size_t i = 0; i < size; ++i)
        {
            raw_vec_mutable.Set(i, other_arr[i]);
        }
        CHECK_THROWS_AS(raw_vec_mutable.Set(size + 1, T(0)), Methane::ArgumentExceptionBase<std::out_of_range>);
        CheckRawVector(raw_vec_mutable, other_arr);
    }

    SECTION("X-coordinate getter and setter")
    {
        CHECK(raw_vec.GetX() == Approx(raw_arr[0]));
        auto new_arr = raw_arr; new_arr[0] = new_value;
        CheckRawVector(RawVector<T, size>(raw_arr).SetX(new_value), new_arr);
    }

    SECTION("Y-coordinate setter")
    {
        CHECK(raw_vec.GetY() == Approx(raw_arr[1]));
        auto new_arr = raw_arr; new_arr[1] = new_value;
        CheckRawVector(RawVector<T, size>(raw_arr).SetY(new_value), new_arr);
    }

    if constexpr (size > 2)
    {
        SECTION("Z-coordinate setter")
        {
            CHECK(raw_vec.GetZ() == Approx(raw_arr[2]));
            auto new_arr = raw_arr; new_arr[2] = new_value;
            CheckRawVector(RawVector<T, size>(raw_arr).SetZ(new_value), new_arr);
        }
    }

    if constexpr (size > 3)
    {
        SECTION("W-coordinate setter")
        {
            CHECK(raw_vec.GetW() == Approx(raw_arr[3]));
            auto new_arr = raw_arr; new_arr[3] = new_value;
            CheckRawVector(RawVector<T, size>(raw_arr).SetW(new_value), new_arr);
        }
    }

    SECTION("Length getter")
    {
        T length = 0;
        for(T component : raw_arr)
            length += component * component;
        length = RoundCast<T>(std::sqrt(length));
        CHECK(raw_vec.GetLength() == Approx(length));
    }
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Comparison", "[vector][compare]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>(T(1), T(1));
    const RawVector<T, size>  raw_vec(raw_arr);
    const RawVector<T, size>  identity_vec(CreateEqualComponents<T, size>(T(1)));

    SECTION("Equality")
    {
        CHECK(RawVector<T, size>(raw_arr) == RawVector<T, size>(raw_arr));
        CHECK_FALSE(RawVector<T, size>(raw_arr) == RawVector<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(RawVector<T, size>(raw_arr) != RawVector<T, size>(raw_arr));
        CHECK(RawVector<T, size>(raw_arr) != RawVector<T, size> (CreateComponents<T, size>(T(1), T(2))));
    }
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Math Operations", "[vector][math]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>(T(1), T(1));
    const RawVector<T, size>  raw_vec(raw_arr);
    const RawVector<T, size>  identity_vec(CreateEqualComponents<T, size>(T(1)));

    SECTION("Addition")
    {
        CheckRawVector(raw_vec + identity_vec, CreateComponents<T, size>(T(2), T(1)));
    }

    SECTION("Inplace addition")
    {
        RawVector<T, size> res_vec(raw_vec);
        res_vec += identity_vec;
        CheckRawVector(res_vec, CreateComponents<T, size>(T(2), T(1)));
    }

    SECTION("Subtraction")
    {
        CheckRawVector(raw_vec - identity_vec, CreateComponents<T, size>(T(0), T(1)));
    }

    SECTION("Inplace subtraction")
    {
        RawVector<T, size> res_vec(raw_vec);
        res_vec -= identity_vec;
        CheckRawVector(res_vec, CreateComponents<T, size>(T(0), T(1)));
    }

    SECTION("Multiplication by scalar")
    {
        CheckRawVector(raw_vec * T(2), CreateComponents<T, size>(T(2), T(2)));
    }

    SECTION("Inplace multiplication by scalar")
    {
        RawVector<T, size> res_vec(raw_vec);
        res_vec *= T(2);
        CheckRawVector(res_vec, CreateComponents<T, size>(T(2), T(2)));
    }

    SECTION("Division by scalar")
    {
        CheckRawVector(RawVector<T, size>(CreateComponents<T, size>(T(2), T(2))) / T(2), raw_arr);
    }

    SECTION("Inplace division by scalar")
    {
        RawVector<T, size> res_vec(CreateComponents<T, size>(T(2), T(2)));
        res_vec /= T(2);
        CheckRawVector(res_vec, raw_arr);
    }

    SECTION("Vectors equality comparison")
    {
        CHECK(RawVector<T, size>(raw_arr) == RawVector<T, size>(raw_arr));
        CHECK_FALSE(RawVector<T, size>(raw_arr) == RawVector<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }

    SECTION("Vectors non-equality comparison")
    {
        CHECK_FALSE(RawVector<T, size>(raw_arr) != RawVector<T, size>(raw_arr));
        CHECK(RawVector<T, size>(raw_arr) != RawVector<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }
}