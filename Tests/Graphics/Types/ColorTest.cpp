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

#include <catch2/catch.hpp>

using namespace Methane::Graphics;
using namespace Methane::Data;

#define COLOR_TYPES_MATRIX \
    ((typename T, size_t size), T, size), \
    (float,    3), (float,    4), \
    (double,   3), (double,   4), \
    (uint32_t, 3), (uint32_t, 4), \
    (int32_t,  3), (int32_t,  4)

template<typename T, size_t size, typename V = T, typename = std::enable_if_t<3 <= size && size <= 4>>
void CheckColor(const Color<T, size>& color, const std::array<V, size>& components)
{
    CHECK(color.GetRed<V>()   == Approx(components[0]));
    CHECK(color.GetGreen<V>() == Approx(components[1]));
    CHECK(color.GetBlue<V>()  == Approx(components[2]));
    if constexpr (size > 3)
        CHECK(color.GetAlpha<V>() == Approx(components[3]));
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