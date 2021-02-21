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

FILE: Tests/UserInterface/Types/ContextTest.cpp
Unit-tests of the User Interface Context

******************************************************************************/

#include "FakeRenderContext.hpp"
#include "UnitTypeCatchHelpers.hpp"

#include <Methane/UserInterface/Context.h>
#include <Methane/Data/TypeInvariants.hpp>

#include <catch2/catch.hpp>

using namespace Methane;
using namespace Methane::Graphics;
using namespace Methane::UserInterface;

static const float    g_dot_to_px_factor = 2.F;
static const uint32_t g_font_resolution_dpi = 96;
static const UnitSize g_frame_size_dot { Units::Dots, 960U, 540U };
static const UnitSize g_frame_size_px  { Units::Pixels, 1920U, 1080U };

TEST_CASE("UI Context Accessors", "[ui][context][accessor]")
{
    FakeRenderContext      render_context({ g_frame_size_px.AsBase() }, g_dot_to_px_factor, g_font_resolution_dpi);
    UserInterface::Context ui_context(render_context);

    SECTION("Get UI render context")
    {
        const UserInterface::Context& const_ui_context = ui_context;
        CHECK(std::addressof(ui_context.GetRenderContext()) == std::addressof(render_context));
        CHECK(std::addressof(const_ui_context.GetRenderContext()) == std::addressof(render_context));
    }

    SECTION("Get UI content scale factor and font DPI")
    {
        CHECK(ui_context.GetDotsToPixelsFactor() == Approx(g_dot_to_px_factor));
        CHECK(ui_context.GetFontResolutionDpi() == g_font_resolution_dpi);
    }

    SECTION("Get UI frame size")
    {
        CHECK(ui_context.GetFrameSize() == g_frame_size_px.AsBase());
        CHECK(ui_context.GetFrameSizeIn<Units::Pixels>() == g_frame_size_px);
        CHECK(ui_context.GetFrameSizeIn<Units::Dots>()   == g_frame_size_dot);
        CHECK(ui_context.GetFrameSizeInUnits(Units::Pixels) == g_frame_size_px);
        CHECK(ui_context.GetFrameSizeInUnits(Units::Dots)   == g_frame_size_dot);
    }
}

TEMPLATE_TEST_CASE("UI Context Convertors of Unit Types", "[ui][context][unit][convert]", ALL_BASE_TYPES)
{
    FakeRenderContext render_context({ g_frame_size_px.AsBase() }, g_dot_to_px_factor, g_font_resolution_dpi);
    const UserInterface::Context ui_context(render_context);

    const UnitType<TestType> item_1pix = CreateUnitItem<TestType>(Units::Pixels, 1);
    const UnitType<TestType> item_2pix = CreateUnitItem<TestType>(Units::Pixels, 2);
    const UnitType<TestType> item_1dot = CreateUnitItem<TestType>(Units::Dots, 1);
    const UnitType<TestType> item_2dot = CreateUnitItem<TestType>(Units::Dots, 2);

    SECTION("Convert from unit Pixels to Pixels")
    {
        CHECK(ui_context.ConvertTo<Units::Pixels>(item_1pix) == item_1pix);
        CHECK(ui_context.ConvertToUnits(item_1pix, Units::Pixels) == item_1pix);
    }

    SECTION("Convert from unit Dots to Pixels")
    {
        CHECK(ui_context.ConvertTo<Units::Pixels>(item_1dot) == item_2pix);
        CHECK(ui_context.ConvertToUnits(item_1dot, Units::Pixels) == item_2pix);
    }

    SECTION("Convert from unit Pixels to Dots")
    {
        CHECK(ui_context.ConvertTo<Units::Dots>(item_2pix) == item_1dot);
        CHECK(ui_context.ConvertToUnits(item_2pix, Units::Dots) == item_1dot);
    }

    SECTION("Convert from unit Dots to Dots")
    {
        CHECK(ui_context.ConvertTo<Units::Dots>(item_2dot) == item_2dot);
        CHECK(ui_context.ConvertToUnits(item_2dot, Units::Dots) == item_2dot);
    }

    SECTION("Convert from base Pixels to Pixels")
    {
        CHECK(ui_context.ConvertTo<Units::Pixels>(item_1pix.AsBase()) == item_1pix);
        CHECK(ui_context.ConvertToUnits(item_1pix.AsBase(), Units::Pixels) == item_1pix);
    }

    SECTION("Convert from base Pixels to Dots")
    {
        CHECK(ui_context.ConvertTo<Units::Dots>(item_2pix.AsBase()) == item_1dot);
        CHECK(ui_context.ConvertToUnits(item_2pix.AsBase(), Units::Dots) == item_1dot);
    }

    if constexpr (Data::TypeInvariants<TestType>::is_floating_point)
    {
        const UnitType<TestType> item_ratio = CreateUnitItem<TestType>(Units::Pixels, 0.001);

        SECTION("Convert from base Ratio to Pixels")
        {
            if constexpr (Data::TypeInvariants<TestType>::type_of == Data::TypeOf::Point)
            {
                const auto result_dots = UnitPoint(Units::Pixels, 23, 25);
                CHECK(ui_context.ConvertRatioTo<Units::Pixels>(item_ratio.AsBase()) == result_dots);
            }
            if constexpr (Data::TypeInvariants<TestType>::type_of == Data::TypeOf::RectSize)
            {
                const auto result_dots = UnitSize(Units::Pixels, 236, 253);
                CHECK(ui_context.ConvertRatioTo<Units::Pixels>(item_ratio.AsBase()) == result_dots);
            }
            if constexpr (Data::TypeInvariants<TestType>::type_of == Data::TypeOf::Rect)
            {
                const auto result_dots = UnitRect(Units::Pixels, 23, 25, 236, 253);
                CHECK(ui_context.ConvertRatioTo<Units::Pixels>(item_ratio.AsBase()) == result_dots);
            }
        }

        SECTION("Convert from base Ratio to Dots")
        {
            if constexpr (Data::TypeInvariants<TestType>::type_of == Data::TypeOf::Point)
            {
                const auto result_dots = UnitPoint(Units::Dots, 12, 12);
                CHECK(ui_context.ConvertRatioTo<Units::Dots>(item_ratio.AsBase()) == result_dots);
            }
            if constexpr (Data::TypeInvariants<TestType>::type_of == Data::TypeOf::RectSize)
            {
                const auto result_dots = UnitSize(Units::Dots, 118, 126);
                CHECK(ui_context.ConvertRatioTo<Units::Dots>(item_ratio.AsBase()) == result_dots);
            }
            if constexpr (Data::TypeInvariants<TestType>::type_of == Data::TypeOf::Rect)
            {
                const auto result_dots = UnitRect(Units::Dots, 12, 12, 118, 126);
                CHECK(ui_context.ConvertRatioTo<Units::Dots>(item_ratio.AsBase()) == result_dots);
            }
        }
    }
}

TEMPLATE_TEST_CASE("UI Context Comparison of Unit Types", "[ui][context][unit][convert]", ALL_BASE_TYPES)
{
    FakeRenderContext            render_context({ g_frame_size_px.AsBase() }, g_dot_to_px_factor, g_font_resolution_dpi);
    const UserInterface::Context ui_context(render_context);

    const UnitType<TestType> item_1pix = CreateUnitItem<TestType>(Units::Pixels, 1);
    const UnitType<TestType> item_2pix = CreateUnitItem<TestType>(Units::Pixels, 2);
    const UnitType<TestType> item_1dot = CreateUnitItem<TestType>(Units::Dots, 1);
    const UnitType<TestType> item_2dot = CreateUnitItem<TestType>(Units::Dots, 2);

    SECTION("Equality comparison of item with same Units")
    {
        CHECK(ui_context.AreEqual(item_1pix, item_1pix));
        CHECK_FALSE(ui_context.AreEqual(item_1pix, item_2pix));
    }

    SECTION("Equality comparison of item with different Units")
    {
        CHECK(ui_context.AreEqual(item_2pix, item_1dot));
        CHECK(ui_context.AreEqual(item_1dot, item_2pix));
        CHECK_FALSE(ui_context.AreEqual(item_1pix, item_2dot));
    }
}

TEMPLATE_TEST_CASE("UI Context Convertors of Scalar Types", "[ui][context][unit][convert]", int32_t, uint32_t, float, double)
{
    FakeRenderContext render_context({ g_frame_size_px.AsBase() }, g_dot_to_px_factor, g_font_resolution_dpi);
    const UserInterface::Context ui_context(render_context);
    const TestType scalar_value = 640;

    SECTION("Convert scalar Dots to Pixels")
    {
        CHECK(ui_context.ConvertDotsToPixels(scalar_value) == scalar_value * static_cast<TestType>(g_dot_to_px_factor));
    }

    SECTION("Convert scalar Pixels to Dots")
    {
        CHECK(ui_context.ConvertPixelsToDots(scalar_value) == scalar_value / static_cast<TestType>(g_dot_to_px_factor));
    }
}