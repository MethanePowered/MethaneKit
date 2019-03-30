/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Test/MouseTest.cpp
Unit tests of the Mouse data types

******************************************************************************/

#include <catch2/catch.hpp>

#include <Methane/Platform/Mouse.h>

using namespace Methane::Platform::Mouse;

static const Position g_test_position = { 12, 34 };

TEST_CASE("Mouse state initialization", "[mouse-state]")
{
    SECTION("Default constructor")
    {
        const State mouse_state;
        const ButtonStates released_button_states{};
        CHECK(mouse_state.GetButtonStates() == released_button_states);
        CHECK(mouse_state.GetPressedButtons() == Buttons());
        CHECK(mouse_state.GetPosition() == Position(0, 0));
    }

    SECTION("Initializer list constructor")
    {
        const State mouse_state = { Button::Left, Button::Right };
        CHECK(mouse_state.GetPressedButtons() == Buttons{ Button::Left, Button::Right });
        CHECK(mouse_state.GetPosition() == Position(0, 0));
    }

    SECTION("Buttons with position constructor")
    {
        const State mouse_state({ Button::Left, Button::Right }, g_test_position);
        CHECK(mouse_state.GetPressedButtons() == Buttons{ Button::Left, Button::Right });
        CHECK(mouse_state.GetPosition() == g_test_position);
    }

    SECTION("Copy constructor")
    {
        const State mouse_state_a({ Button::Left, Button::Right }, g_test_position);
        const State mouse_state_b(mouse_state_a);
        CHECK(mouse_state_b.GetPressedButtons() == Buttons{ Button::Left, Button::Right });
        CHECK(mouse_state_b.GetPosition() == g_test_position);
    }
}

TEST_CASE("Mouse state modification", "[mouse-state]")
{
    SECTION("Press buttons")
    {
        State mouse_state;
        mouse_state.PressButton(Button::Left);
        mouse_state.PressButton(Button::Middle);
        mouse_state.PressButton(Button::Right);
        CHECK(mouse_state.GetPressedButtons() == Buttons{ Button::Left, Button::Middle, Button::Right });
    }

    SECTION("Release buttons")
    {
        State mouse_state = { Button::Left, Button::Middle, Button::Right };
        mouse_state.ReleaseButton(Button::Left);
        mouse_state.ReleaseButton(Button::Right);
        CHECK(mouse_state.GetPressedButtons() == Buttons{ Button::Middle });
    }

    SECTION("Set position")
    {
        State mouse_state = { Button::Left };
        mouse_state.SetPosition(g_test_position);
        CHECK(mouse_state.GetPosition() == g_test_position);
    }
}

TEST_CASE("Mouse state comparison", "[mouse-state]")
{
    SECTION("States equality")
    {
        const State mouse_state_a({ Button::Left }, g_test_position);
        const State mouse_state_b({ Button::Left }, g_test_position);
        CHECK(mouse_state_a == mouse_state_b);
    }

    SECTION("States inequality in buttons")
    {
        const State mouse_state_a({ Button::Left },  g_test_position);
        const State mouse_state_b({ Button::Right }, g_test_position);
        CHECK(mouse_state_a != mouse_state_b);
    }

    SECTION("States inequality in position")
    {
        const State mouse_state_a({ Button::Left }, g_test_position);
        const State mouse_state_b({ Button::Left }, { 56, 78 });
        CHECK(mouse_state_a != mouse_state_b);
    }
}