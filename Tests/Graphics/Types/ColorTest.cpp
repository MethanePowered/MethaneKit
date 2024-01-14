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

FILE: Tests/Graphics/Types/ColorTest.cpp
Unit-tests of the Color data type wrapping HLSL++ vector

******************************************************************************/

#include <Methane/Graphics/Color.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace Methane::Graphics;
using namespace Methane::Data;
using Catch::Approx;

#define COLOR_TYPES_MATRIX \
    ((typename T, size_t size), T, size), \
    (float,    3), (float,    4), \
    (double,   3), (double,   4), \
    (uint32_t, 3), (uint32_t, 4), \
    (int32_t,  3), (int32_t,  4)

template<typename T, size_t size, typename V = T, typename = std::enable_if_t<3 <= size && size <= 4>>
void CheckColor(const Color<T, size>& color, const std::array<V, size>& components, float epsilon = std::numeric_limits<float>::epsilon() * 100.f)
{
    CHECK(color.template GetRed<V>()   == Approx(components[0]).epsilon(epsilon));
    CHECK(color.template GetGreen<V>() == Approx(components[1]).epsilon(epsilon));
    CHECK(color.template GetBlue<V>()  == Approx(components[2]).epsilon(epsilon));
    if constexpr (size > 3)
        CHECK(color.template GetAlpha<V>() == Approx(components[3]).epsilon(epsilon));
}

template<typename T, size_t size>
std::array<T, size> CreateColorComponents(double step_ratio = 1.0 / (size + 1))
{
    const T component_max  = Color<T, size>::GetComponentMax();
    const T component_step = RoundCast<T>(static_cast<double>(component_max) * step_ratio);
    std::array<T, size> values{ component_step };
    for (size_t i = 1; i < size; ++i)
    {
        values[i] = component_step * T(i + 1);
        CHECK(values[i] <= component_max);
    }
    return values;
}

template<typename T>
uint8_t GetColorComponentAsByte(T color_component)
{
    if constexpr (std::is_floating_point_v<T>)
        return RoundCast<uint8_t>(color_component * 255.0);
    else
        return RoundCast<uint8_t>(static_cast<double>(color_component) * 255.0 / static_cast<double>(std::numeric_limits<T>::max()));
}

TEMPLATE_TEST_CASE_SIG("Color Initialization", "[color][init]", COLOR_TYPES_MATRIX)
{
    const auto test_color_arr = CreateColorComponents<T, size>();

    SECTION("Default initialization with zeros")
    {
        CheckColor(Color<T, size>(), { });
    }

    SECTION("Initialization with array of components")
    {
        CheckColor(Color<T, size>(test_color_arr), test_color_arr);
    }

    SECTION("Initialization with all components of original type")
    {
        if constexpr (size == 3)
            CheckColor(Color<T, 3>(test_color_arr[0], test_color_arr[1], test_color_arr[2]), test_color_arr);
        else
            CheckColor(Color<T, 4>(test_color_arr[0], test_color_arr[1], test_color_arr[2], test_color_arr[3]), test_color_arr);
    }

    if constexpr (std::is_floating_point_v<T>)
    {
        SECTION("Initialization with all components of unsigned integer type")
        {
            const auto uint_color_arr = CreateColorComponents<uint32_t, size>();
            if constexpr (size == 3)
                CheckColor(Color<T, 3>(uint_color_arr[0], uint_color_arr[1], uint_color_arr[2]), uint_color_arr);
            else
                CheckColor(Color<T, 4>(uint_color_arr[0], uint_color_arr[1], uint_color_arr[2], uint_color_arr[3]), uint_color_arr);
        }

        SECTION("Exception on initialization with components greater than 1.0")
        {
            if constexpr (size == 3)
            {
                CHECK_THROWS(Color<T, 3>(2.0, test_color_arr[1], test_color_arr[2]));
                CHECK_THROWS(Color<T, 3>(test_color_arr[0], 2.0, test_color_arr[2]));
                CHECK_THROWS(Color<T, 3>(test_color_arr[0], test_color_arr[1], 2.0));
            }
            else
            {
                CHECK_THROWS(Color<T, 4>(2.0, test_color_arr[1], test_color_arr[2], test_color_arr[3]));
                CHECK_THROWS(Color<T, 4>(test_color_arr[0], 2.0, test_color_arr[2], test_color_arr[3]));
                CHECK_THROWS(Color<T, 4>(test_color_arr[0], test_color_arr[1], 2.0, test_color_arr[3]));
                CHECK_THROWS(Color<T, 4>(test_color_arr[0], test_color_arr[1], test_color_arr[2], 2.0));
            }
        }
    }
    else
    {
        SECTION("Initialization with all components of floating point type")
        {
            const auto float_color_arr = CreateColorComponents<float, size>();
            if constexpr (size == 3)
                CheckColor(Color<T, 3>(float_color_arr[0], float_color_arr[1], float_color_arr[2]), float_color_arr);
            else
                CheckColor(Color<T, 4>(float_color_arr[0], float_color_arr[1], float_color_arr[2], float_color_arr[3]), float_color_arr);
        }
    }

    SECTION("Initialization with components of mixed types")
    {
        const auto float_components = CreateColorComponents<float, size>();
        const auto uint_components  = CreateColorComponents<uint32_t, size>();
        Color<T, size> test_color;
        if constexpr (size == 3)
            test_color = Color<T, 3>(float_components[0], float_components[1], uint_components[2]);
        else
            test_color = Color<T, 4>(uint_components[0], uint_components[1], float_components[2], float_components[3]);

        CheckColor(test_color, float_components);
        CheckColor(test_color, uint_components);
    }

    if constexpr (size == 4)
    {
        SECTION("Initialization with 3-component color and extra component")
        {
            Color<T, 3> small_color(test_color_arr[0], test_color_arr[1], test_color_arr[2]);
            CheckColor(Color<T, 4>(small_color, test_color_arr[3]), test_color_arr);
        }
    }

    SECTION("Initialization with HLSL vector reference")
    {
        if constexpr (size == 3)
        {
            const HlslVector<T, 3> hlsl_vector(test_color_arr[0], test_color_arr[1], test_color_arr[2]);
            CheckColor(Color<T, 3>(hlsl_vector), test_color_arr);
        }
        else
        {
            const HlslVector<T, 4> hlsl_vector(test_color_arr[0], test_color_arr[1], test_color_arr[2], test_color_arr[3]);
            CheckColor(Color<T, 4>(hlsl_vector), test_color_arr);
        }
    }

    SECTION("Initialization with moved HLSL vector move")
    {
        if constexpr (size == 3)
        {
            HlslVector<T, 3> hlsl_vector(test_color_arr[0], test_color_arr[1], test_color_arr[2]);
            CheckColor(Color<T, 3>(std::move(hlsl_vector)), test_color_arr);
        }
        else
        {
            HlslVector<T, 4> hlsl_vector(test_color_arr[0], test_color_arr[1], test_color_arr[2], test_color_arr[3]);
            CheckColor(Color<T, 4>(std::move(hlsl_vector)), test_color_arr);
        }
    }

    SECTION("Copy initialization from the same color type")
    {
        const Color<T, size> color(test_color_arr);
        CheckColor(Color<T, size>(color), test_color_arr);
    }

    SECTION("Move initialization from the same color type")
    {
        Color<T, size> color(test_color_arr);
        CheckColor(Color<T, size>(std::move(test_color_arr)), test_color_arr);
    }

    SECTION("Copy assignment initialization")
    {
        const Color<T, size> color(test_color_arr);
        Color<T, size> copy_color;
        copy_color = color;
        CheckColor(copy_color, test_color_arr);
    }

    SECTION("Move assignment initialization")
    {
        Color<T, size> color(test_color_arr);
        Color<T, size> copy_color = std::move(color);
        CheckColor(copy_color, test_color_arr);
    }
}

TEMPLATE_TEST_CASE_SIG("Color Component Accessors", "[color][accessors]", COLOR_TYPES_MATRIX)
{
    const auto test_color_arr = CreateColorComponents<T, size>();
    const Color<T, size> test_color(test_color_arr);

    SECTION("Red component getter")
    {
        CHECK(test_color.GetRed() == Approx(test_color_arr[0]));
        CHECK(test_color.template GetRed<uint8_t>() == GetColorComponentAsByte(test_color_arr[0]));
    }

    SECTION("Green component getter")
    {
        CHECK(test_color.GetGreen() == Approx(test_color_arr[1]));
        CHECK(test_color.template GetGreen<uint8_t>() == GetColorComponentAsByte(test_color_arr[1]));
    }

    SECTION("Blue component getter")
    {
        CHECK(test_color.GetBlue() == Approx(test_color_arr[2]));
        CHECK(test_color.template GetBlue<uint8_t>() == GetColorComponentAsByte(test_color_arr[2]));
    }

    if constexpr (size == 4)
    {
        SECTION("Alpha component getter")
        {
            CHECK(test_color.GetAlpha() == Approx(test_color_arr[3]));
            CHECK(test_color.template GetAlpha<uint8_t>() == GetColorComponentAsByte(test_color_arr[3]));
        }
    }

    SECTION("Indexed component getters")
    {
        for(size_t component_index = 0; component_index < size; ++component_index)
        {
            CHECK(test_color[component_index] == Approx(test_color_arr[component_index]));
            CHECK(test_color.Get(component_index) == Approx(test_color_arr[component_index]));
            CHECK(test_color.template Get<uint8_t>(component_index) == GetColorComponentAsByte(test_color_arr[component_index]));
        }
    }

    SECTION("Component channel setters with original type")
    {
        Color<T, size> color;
        CHECK_NOTHROW(color.SetRed(test_color_arr[0]));
        CHECK_NOTHROW(color.SetGreen(test_color_arr[1]));
        CHECK_NOTHROW(color.SetBlue(test_color_arr[2]));
        if constexpr (size == 4)
            CHECK_NOTHROW(color.SetAlpha(test_color_arr[3]));
        CheckColor(color, test_color_arr);
    }

    if constexpr (std::is_floating_point_v<T>)
    {
        SECTION("Exception on channel set with value greater 1.0")
        {
            Color<T, size> color(test_color_arr);
            CHECK_THROWS_AS(color.SetRed(2.0), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(color.SetGreen(2.0), Methane::ArgumentExceptionBase<std::out_of_range>);
            CHECK_THROWS_AS(color.SetBlue(2.0), Methane::ArgumentExceptionBase<std::out_of_range>);
            if constexpr (size == 4)
                CHECK_THROWS_AS(color.SetAlpha(2.0), Methane::ArgumentExceptionBase<std::out_of_range>);
            CheckColor(color, test_color_arr);
        }

        SECTION("Exception on indexed setter with value greater 1.0")
        {
            Color<T, size> color(test_color_arr);
            for(size_t component_index = 0; component_index < color.GetSize(); ++component_index)
            {
                CHECK_THROWS_AS(color.Set(component_index, 2.0), Methane::ArgumentExceptionBase<std::out_of_range>);
            }
            CheckColor(color, test_color_arr);
        }
    }

    SECTION("Component channel setters with byte")
    {
        Color<T, size> color;
        CHECK_NOTHROW(color.template SetRed<uint8_t>(GetColorComponentAsByte(test_color_arr[0])));
        CHECK_NOTHROW(color.template SetGreen<uint8_t>(GetColorComponentAsByte(test_color_arr[1])));
        CHECK_NOTHROW(color.template SetBlue<uint8_t>(GetColorComponentAsByte(test_color_arr[2])));
        if constexpr (size == 4)
            CHECK_NOTHROW(color.template SetAlpha<uint8_t>(GetColorComponentAsByte(test_color_arr[3])));
        CheckColor(color, test_color_arr, 0.01F);
    }

    SECTION("Indexed component setters with original type")
    {
        Color<T, size> color;
        for(size_t component_index = 0; component_index < color.GetSize(); ++component_index)
        {
            CHECK_NOTHROW(color.Set(component_index, test_color_arr[component_index]));
        }
        CheckColor(color, test_color_arr);
    }

    SECTION("Indexed component setters with byte")
    {
        Color<T, size> color;
        for(size_t component_index = 0; component_index < color.GetSize(); ++component_index)
        {
            CHECK_NOTHROW(color.template Set<uint8_t>(component_index, GetColorComponentAsByte(test_color_arr[component_index])));
        }
        CheckColor(color, test_color_arr, 0.01F);
    }
}

TEMPLATE_TEST_CASE_SIG("Color Conversions to Other Types", "[color][convert]", COLOR_TYPES_MATRIX)
{
    const auto test_color_arr = CreateColorComponents<T, size>();
    const Color<T, size> test_color(test_color_arr);

    SECTION("Convert to array")
    {
        CHECK(test_color.AsArray() == test_color_arr);
    }

    SECTION("Convert to HLSL vector")
    {
        CHECK(hlslpp::all(test_color.AsVector() == CreateHlslVector(test_color_arr)));
    }

    SECTION("Cast to HLSL vector")
    {
        CHECK(hlslpp::all(static_cast<HlslVector<T, size>>(test_color) == CreateHlslVector(test_color_arr)));
    }

    SECTION("Cast to color of other type")
    {
        if constexpr (std::is_floating_point_v<T>)
            CheckColor(static_cast<Color<uint32_t, size>>(test_color), test_color_arr);
        else
            CheckColor(static_cast<Color<float, size>>(test_color), test_color_arr);
    }

    SECTION("Cast to string")
    {
        if constexpr (size == 3)
            CHECK(static_cast<std::string>(test_color) == "C(r:64, g:125, b:191)");
        else
            CHECK(static_cast<std::string>(test_color) == "C(r:51, g:101, b:153, a:204)");
    }
}

TEMPLATE_TEST_CASE_SIG("Color Comparison", "[color][compare]", COLOR_TYPES_MATRIX)
{
    const auto test_color_arr = CreateColorComponents<T, size>();
    const Color<T, size> test_color(test_color_arr);

    SECTION("Equality")
    {
        CHECK(test_color == Color<T, size>(test_color_arr));
        CHECK_FALSE(test_color == Color<T, size>(test_color_arr).template SetRed<double>(1.0));
        CHECK_FALSE(test_color == Color<T, size>(test_color_arr).template SetGreen<double>(1.0));
        CHECK_FALSE(test_color == Color<T, size>(test_color_arr).template SetBlue<double>(1.0));
        if constexpr (size == 4)
            CHECK_FALSE(test_color == Color<T, size>(test_color_arr).template SetAlpha<double>(1.0));
        CHECK_FALSE(test_color == Color<T, size>());
    }

    SECTION("Inequality")
    {
        CHECK_FALSE(test_color != Color<T, size>(test_color_arr));
        CHECK(test_color != Color<T, size>(test_color_arr).template SetRed<double>(1.0));
        CHECK(test_color != Color<T, size>(test_color_arr).template SetGreen<double>(1.0));
        CHECK(test_color != Color<T, size>(test_color_arr).template SetBlue<double>(1.0));
        if constexpr (size == 4)
            CHECK(test_color != Color<T, size>(test_color_arr).template SetAlpha<double>(1.0));
        CHECK(test_color != Color<T, size>());
    }
}