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

FILE: Test/KeyboardTest.cpp
Unit tests of the Keyboard data types

******************************************************************************/

#include <catch2/catch.hpp>

#include <Methane/Platform/Keyboard.h>

using namespace Methane::Platform::Keyboard;

TEST_CASE("Keyboard state initialization", "[keyboard-state]")
{
    SECTION("Default constructor")
    {
        const State keyboard_state;
        const KeyStates released_key_states{};
        CHECK(keyboard_state.GetKeyStates() == released_key_states);
        CHECK(keyboard_state.GetPressedKeys() == Keys{});
        CHECK(keyboard_state.GetModifiersMask() == Modifier::None);
    }

    SECTION("Initializer list constructor")
    {
        const State keyboard_state = { Key::LeftControl, Key::LeftShift, Key::C };
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::C });
        CHECK(keyboard_state.GetModifiersMask() == (Modifier::Control | Modifier::Shift));
    }

    SECTION("Copy constructor")
    {
        const State keyboard_state_a = { Key::LeftControl, Key::LeftShift, Key::C, Key::Up };
        const State keyboard_state_b(keyboard_state_a);
        CHECK(keyboard_state_b.GetPressedKeys() == Keys{ Key::C, Key::Up });
        CHECK(keyboard_state_b.GetModifiersMask() == (Modifier::Control | Modifier::Shift));
    }
    
    SECTION("Construct with unknown key")
    {
        const State keyboard_state = { Key::Unknown };
        CHECK(keyboard_state.GetPressedKeys() == Keys{ });
        CHECK(keyboard_state.GetModifiersMask() == Modifier::None);
    }
}

TEST_CASE("Keyboard state modification", "[keyboard-state]")
{
    SECTION("Press printable key")
    {
        State keyboard_state;
        keyboard_state.PressKey(Key::A);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::A });
        CHECK(keyboard_state.GetModifiersMask() == Modifier::None);
    }

    SECTION("Press control key")
    {
        State keyboard_state;
        keyboard_state.PressKey(Key::LeftAlt);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ });
        CHECK(keyboard_state.GetModifiersMask() == Modifier::Alt);
    }

    SECTION("Release printable key")
    {
        State keyboard_state = { Key::RightControl, Key::RightAlt, Key::W, Key::Num3 };
        keyboard_state.ReleaseKey(Key::Num3);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::W });
        CHECK(keyboard_state.GetModifiersMask() == (Modifier::Control | Modifier::Alt));
    }

    SECTION("Release control key")
    {
        State keyboard_state = { Key::RightControl, Key::RightAlt, Key::W, Key::Num3 };
        keyboard_state.ReleaseKey(Key::RightAlt);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::W, Key::Num3 });
        CHECK(keyboard_state.GetModifiersMask() == Modifier::Control);
    }
}

TEST_CASE("Keyboard state comparison", "[keyboard-state]")
{
    SECTION("States equality")
    {
        const State keyboard_state_a = { Key::RightControl, Key::LeftAlt,  Key::Up, Key::Y, Key::Num5 };
        const State keyboard_state_b = { Key::LeftControl,  Key::RightAlt, Key::Up, Key::Y, Key::Num5 };
        CHECK(keyboard_state_a == keyboard_state_b);
        CHECK(keyboard_state_a.GetDiff(keyboard_state_b) == State::Property::None);
    }

    SECTION("States inequality in printable keys")
    {
        const State keyboard_state_a = { Key::RightControl, Key::LeftAlt,   Key::Down, Key::U, Key::Num2 };
        const State keyboard_state_b = { Key::LeftControl,  Key::RightAlt,  Key::Up,   Key::Y, Key::Num5 };
        CHECK(keyboard_state_a != keyboard_state_b);
        CHECK(keyboard_state_a.GetDiff(keyboard_state_b) == State::Property::KeyStates);
    }

    SECTION("States inequality in modifiers")
    {
        const State keyboard_state_a = { Key::RightControl, Key::LeftShift, Key::Up, Key::Y, Key::Num5 };
        const State keyboard_state_b = { Key::LeftControl,  Key::RightAlt,  Key::Up, Key::Y, Key::Num5 };
        CHECK(keyboard_state_a != keyboard_state_b);
        CHECK(keyboard_state_a.GetDiff(keyboard_state_b) == State::Property::Modifiers);
    }
}
