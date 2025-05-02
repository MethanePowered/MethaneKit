/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Tests/Graphics/RHI/ViewStateTest.cpp
Unit-tests of the RHI ViewState

******************************************************************************/

#include <Methane/Graphics/RHI/ViewState.h>

#include <memory>
#include <catch2/catch_test_macros.hpp>

using namespace Methane;
using namespace Methane::Graphics;

TEST_CASE("RHI View State Functions", "[rhi][view][state]")
{
    const Rhi::ViewSettings view_state_settings{
        .viewports = {
            Viewport(1.2, 2.3, 3.4, 5.1, 6.2, 7.3),
            Viewport(2.4, 3.5, 4.6, 6.3, 7.4, 8.5),
            Viewport(3.6, 4.7, 5.8, 7.5, 8.6, 9.7)
        },
        .scissor_rects = {
            ScissorRect(0U, 1U, 2U, 3U),
            ScissorRect(1U, 2U, 3U, 4U),
            ScissorRect(2U, 3U, 4U, 5U)
        }
    };

    SECTION("View State Construction")
    {
        Rhi::ViewState view_state;
        REQUIRE_NOTHROW(view_state = Rhi::ViewState(view_state_settings));
        REQUIRE(view_state.IsInitialized());
        CHECK(view_state.GetInterfacePtr());
        CHECK(view_state.GetSettings().viewports == view_state_settings.viewports);
        CHECK(view_state.GetSettings().scissor_rects == view_state_settings.scissor_rects);
    }

    Rhi::ViewState view_state(view_state_settings);
    const Rhi::ViewSettings new_settings{
        .viewports = {
            Viewport(9.1, 8.2, 7.3, 6.2, 5.1, 4.0)
        },
        .scissor_rects = {
            ScissorRect(5U, 6U, 8U, 7U)
        }
    };

    SECTION("Reset with Settings")
    {
        REQUIRE_NOTHROW(view_state.Reset(new_settings));
        REQUIRE(view_state.GetSettings().viewports == new_settings.viewports);
        REQUIRE(view_state.GetSettings().scissor_rects == new_settings.scissor_rects);
    }

    SECTION("Set Viewports")
    {
        REQUIRE_NOTHROW(view_state.SetViewports(new_settings.viewports));
        REQUIRE(view_state.GetSettings().viewports == new_settings.viewports);
        REQUIRE(view_state.GetSettings().scissor_rects == view_state_settings.scissor_rects);
    }

    SECTION("Set Scissor Rects")
    {
        REQUIRE_NOTHROW(view_state.SetScissorRects(new_settings.scissor_rects));
        REQUIRE(view_state.GetSettings().viewports == view_state_settings.viewports);
        REQUIRE(view_state.GetSettings().scissor_rects == new_settings.scissor_rects);
    }

    SECTION("Convert Settings to String")
    {
        CHECK(static_cast<std::string>(view_state_settings) ==
              "  - Viewports: Vol[P(1.2, 2.3, 3.4) : Sz(5.1 x 6.2 x 7.3)], Vol[P(2.4, 3.5, 4.6) : Sz(6.3 x 7.4 x 8.5)], Vol[P(3.6, 4.7, 5.8) : Sz(7.5 x 8.6 x 9.7)];\n"
              "  - Scissor Rects: Rect[P(0, 1) : Sz(2 x 3)], Rect[P(1, 2) : Sz(3 x 4)], Rect[P(2, 3) : Sz(4 x 5)].");
    }
}
