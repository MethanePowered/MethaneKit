/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Test/KeyboardTest.cpp
Unit tests of the Keyboard data types

******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <Methane/Platform/Input/Keyboard.h>
#include <Methane/Data/EnumMaskUtil.hpp>

using namespace Methane::Platform::Input::Keyboard;

template<typename E, typename M>
struct Catch::StringMaker<Methane::Data::EnumMask<E, M>>
{
    static std::string convert(const Methane::Data::EnumMask<E, M>& mask)
    {
        return Methane::Data::GetEnumMaskName(mask);
    }
};

TEST_CASE("Keyboard state initialization", "[keyboard-state]")
{
    SECTION("Default constructor")
    {
        const State keyboard_state;
        const KeyStates released_key_states{};
        CHECK(keyboard_state.GetKeyStates() == released_key_states);
        CHECK(keyboard_state.GetPressedKeys() == Keys{});
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask{});
    }

    SECTION("Initializer list constructor")
    {
        const State keyboard_state{ Key::LeftControl, Key::LeftShift, Key::C };
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::C });
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask({ Modifier::Control, Modifier::Shift }));
    }

    SECTION("Copy constructor")
    {
        const State keyboard_state_a{ Key::LeftControl, Key::LeftShift, Key::C, Key::Up };
        const State keyboard_state_b(keyboard_state_a);
        CHECK(keyboard_state_b.GetPressedKeys() == Keys{ Key::C, Key::Up });
        CHECK(keyboard_state_b.GetModifiersMask() == ModifierMask({ Modifier::Control, Modifier::Shift }));
    }
    
    SECTION("Construct with unknown key")
    {
        const State keyboard_state{ Key::Unknown };
        CHECK(keyboard_state.GetPressedKeys() == Keys{ });
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask{});
    }
}

TEST_CASE("Keyboard state modification", "[keyboard-state]")
{
    SECTION("Press printable key")
    {
        State keyboard_state;
        keyboard_state.PressKey(Key::A);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::A });
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask{});
    }

    SECTION("Press control key")
    {
        State keyboard_state;
        keyboard_state.PressKey(Key::LeftAlt);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ });
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask(Modifier::Alt));
    }

    SECTION("Release printable key")
    {
        State keyboard_state{ Key::RightControl, Key::RightAlt, Key::W, Key::Num3 };
        keyboard_state.ReleaseKey(Key::Num3);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::W });
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask({ Modifier::Control, Modifier::Alt }));
    }

    SECTION("Release control key")
    {
        State keyboard_state{ Key::RightControl, Key::RightAlt, Key::W, Key::Num3 };
        keyboard_state.ReleaseKey(Key::RightAlt);
        CHECK(keyboard_state.GetPressedKeys() == Keys{ Key::W, Key::Num3 });
        CHECK(keyboard_state.GetModifiersMask() == ModifierMask(Modifier::Control));
    }
}

TEST_CASE("Keyboard state comparison", "[keyboard-state]")
{
    SECTION("States equality")
    {
        const State keyboard_state_a{ Key::RightControl, Key::LeftAlt,  Key::Up, Key::Y, Key::Num5 };
        const State keyboard_state_b{ Key::LeftControl,  Key::RightAlt, Key::Up, Key::Y, Key::Num5 };
        CHECK(keyboard_state_a == keyboard_state_b);
        CHECK(keyboard_state_a.GetDiff(keyboard_state_b) == State::PropertyMask{});
    }

    SECTION("States inequality in printable keys")
    {
        const State keyboard_state_a{ Key::RightControl, Key::LeftAlt,   Key::Down, Key::U, Key::Num2 };
        const State keyboard_state_b{ Key::LeftControl,  Key::RightAlt,  Key::Up,   Key::Y, Key::Num5 };
        CHECK(keyboard_state_a != keyboard_state_b);
        CHECK(keyboard_state_a.GetDiff(keyboard_state_b) == State::PropertyMask(State::Property::KeyStates));
    }

    SECTION("States inequality in modifiers")
    {
        const State keyboard_state_a{ Key::RightControl, Key::LeftShift, Key::Up, Key::Y, Key::Num5 };
        const State keyboard_state_b{ Key::LeftControl,  Key::RightAlt,  Key::Up, Key::Y, Key::Num5 };
        CHECK(keyboard_state_a != keyboard_state_b);
        CHECK(keyboard_state_a.GetDiff(keyboard_state_b) == State::PropertyMask(State::Property::Modifiers));
    }
}

TEST_CASE("Keyboard state getters and converters", "[keyboard-state]")
{
    SECTION("Key state getter")
    {
        const StateExt keyboard_state{ Key::RightControl, Key::LeftShift, Key::Up, Key::Y, Key::Num5, Key::KeyPad3, Key::F7 };
        CHECK(keyboard_state.GetModifiersMask().HasAnyBit(Modifier::Control));
        CHECK(keyboard_state.GetModifiersMask().HasAnyBit(Modifier::Shift));
        CHECK(keyboard_state[Key::Up] == KeyState::Pressed);
        CHECK(keyboard_state[Key::Down] == KeyState::Released);
        CHECK(keyboard_state[Key::Y] == KeyState::Pressed);
        CHECK(keyboard_state[Key::Z] == KeyState::Released);
        CHECK(keyboard_state[Key::Num5] == KeyState::Pressed);
        CHECK(keyboard_state[Key::Num4] == KeyState::Released);
        CHECK(keyboard_state[Key::KeyPad3] == KeyState::Pressed);
        CHECK(keyboard_state[Key::KeyPad2] == KeyState::Released);
        CHECK(keyboard_state[Key::F7] == KeyState::Pressed);
        CHECK(keyboard_state[Key::F8] == KeyState::Released);
    }

    SECTION("State with modifiers pressed conversion to boolean")
    {
        const State keyboard_state({}, ModifierMask({ Modifier::Control, Modifier::Alt }));
        CHECK(static_cast<bool>(keyboard_state));
    }

    SECTION("State with keys pressed conversion to boolean")
    {
        const State keyboard_state({ Key::Enter });
        CHECK(static_cast<bool>(keyboard_state));
    }
}
