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

FILE: Methane/Platform/Keyboard.cpp
Platform abstraction of keyboard events.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <map>
#include <sstream>

namespace Methane::Platform::Keyboard
{

static const std::string g_keys_separator       = "+";
static const std::string g_properties_separator = "+";

KeyConverter::KeyConverter(Key key)
    : m_key(key)
    , m_modifiers(GetModifierKey())
{
    META_FUNCTION_TASK();
}

KeyConverter::KeyConverter(Key key, Modifiers modifiers)
    : m_key(key)
    , m_modifiers(modifiers)
{
    META_FUNCTION_TASK();
}

KeyConverter::KeyConverter(const NativeKey& native_key)
    : m_key(GetKeyByNativeCode(native_key))
    , m_modifiers(GetModifiersByNativeCode(native_key))
{
    META_FUNCTION_TASK();
}

Modifiers KeyConverter::GetModifierKey() const noexcept
{
    META_FUNCTION_TASK();
    switch (m_key)
    {
    case Key::LeftShift:
    case Key::RightShift:
        return Modifiers::Shift;

    case Key::LeftControl:
    case Key::RightControl:
        return Modifiers::Control;

    case Key::LeftAlt:
    case Key::RightAlt:
        return Modifiers::Alt;

    case Key::LeftSuper:
    case Key::RightSuper:
        return Modifiers::Super;

    case Key::CapsLock:
        return Modifiers::CapsLock;
    case Key::NumLock:
        return Modifiers::NumLock;

    default:
        return Modifiers::None;
    }
}

std::string KeyConverter::ToString() const
{
    META_FUNCTION_TASK();
    static const std::map<Key, std::string> s_name_by_key =
    {
        // Printable keys
        { Key::Space, "SPACE" },
        { Key::Apostrophe, "'" },
        { Key::Comma, "," },
        { Key::Minus, "-" },
        { Key::Period, "." },
        { Key::Slash, "/" },
        { Key::Num0, "0" },
        { Key::Num1, "1" },
        { Key::Num2, "2" },
        { Key::Num3, "3" },
        { Key::Num4, "4" },
        { Key::Num5, "5" },
        { Key::Num6, "6" },
        { Key::Num7, "7" },
        { Key::Num8, "8" },
        { Key::Num9, "9" },
        { Key::Semicolon, ";" },
        { Key::Equal, "=" },
        { Key::A, "A" },
        { Key::B, "B" },
        { Key::C, "C" },
        { Key::D, "D" },
        { Key::E, "E" },
        { Key::F, "F" },
        { Key::G, "G" },
        { Key::H, "H" },
        { Key::I, "I" },
        { Key::J, "J" },
        { Key::K, "K" },
        { Key::L, "L" },
        { Key::M, "M" },
        { Key::N, "N" },
        { Key::O, "O" },
        { Key::P, "P" },
        { Key::Q, "Q" },
        { Key::R, "R" },
        { Key::S, "S" },
        { Key::T, "T" },
        { Key::U, "U" },
        { Key::V, "V" },
        { Key::W, "W" },
        { Key::X, "X" },
        { Key::Y, "Y" },
        { Key::Z, "Z" },
        { Key::LeftBracket, "[" },
        { Key::BackSlash, "\\" },
        { Key::RightBracket, "]" },
        { Key::GraveAccent, "`" },
        { Key::World1, "W1" },
        { Key::World2, "W2" },

        // Function Keys
        { Key::Escape, "ESC" },
        { Key::Enter, "ENTER" },
        { Key::Tab, "TAB" },
        { Key::Backspace, "BACKSPACE" },
        { Key::Insert, "INSERT" },
        { Key::Delete, "DELETE" },
        { Key::Right, "RIGHT" },
        { Key::Left, "LEFT" },
        { Key::Down, "DOWN" },
        { Key::Up, "UP" },
        { Key::PageUp, "PAGEUP" },
        { Key::PageDown, "PAGEDOWN" },
        { Key::Home, "HOME" },
        { Key::End, "END" },
        { Key::CapsLock, "CAPSLOCK" },
        { Key::ScrollLock, "SCROLLOCK" },
        { Key::NumLock, "NUMLOCK" },
        { Key::PrintScreen, "PRINTSCREEN" },
        { Key::Pause, "PAUSE" },
        { Key::F1, "F1" },
        { Key::F2, "F2" },
        { Key::F3, "F3" },
        { Key::F4, "F4" },
        { Key::F5, "F5" },
        { Key::F6, "F6" },
        { Key::F7, "F7" },
        { Key::F8, "F8" },
        { Key::F9, "F9" },
        { Key::F10, "F10" },
        { Key::F11, "F11" },
        { Key::F12, "F12" },
        { Key::F13, "F13" },
        { Key::F14, "F14" },
        { Key::F15, "F15" },
        { Key::F16, "F16" },
        { Key::F17, "F17" },
        { Key::F18, "F18" },
        { Key::F19, "F19" },
        { Key::F20, "F20" },
        { Key::F21, "F21" },
        { Key::F22, "F22" },
        { Key::F23, "F23" },
        { Key::F24, "F24" },
        { Key::F25, "F25" },
        { Key::KeyPad0, "KEYPAD0" },
        { Key::KeyPad1, "KEYPAD0" },
        { Key::KeyPad2, "KEYPAD0" },
        { Key::KeyPad3, "KEYPAD0" },
        { Key::KeyPad4, "KEYPAD0" },
        { Key::KeyPad5, "KEYPAD0" },
        { Key::KeyPad6, "KEYPAD0" },
        { Key::KeyPad7, "KEYPAD0" },
        { Key::KeyPad8, "KEYPAD0" },
        { Key::KeyPad9, "KEYPAD0" },
        { Key::KeyPadDecimal, "KEYPAD." },
        { Key::KeyPadDivide, "KEYPAD/" },
        { Key::KeyPadMultiply, "KEYPAD*" },
        { Key::KeyPadSubtract, "KEYPAD-" },
        { Key::KeyPadAdd, "KEYPAD+" },
        { Key::KeyPadEnter, "KEYPAD_ENTER" },
        { Key::KeyPadEqual, "KEYPAD=" },

        // Control keys
        { Key::LeftShift, "LEFT_SHIFT" },
        { Key::RightShift, "RIGHT_SHIFT" },
        { Key::LeftControl, "LEFT_CONTROL" },
        { Key::RightControl, "RIGHT_CONTROL" },
        { Key::LeftAlt, "LEFT_ALT" },
        { Key::RightAlt, "RIGHT_ALT" },
#ifdef __APPLE__
        { Key::LeftSuper, "LEFT_COMMAND" },
        { Key::RightSuper, "RIGHT_COMMAND" },
#else
        { Key::LeftSuper,       "LEFT_SUPER"    },
        { Key::RightSuper,      "RIGHT_SUPER"   },
#endif
        { Key::Menu, "MENU" },

        { Key::Unknown, "Unknown" }
    };

    auto key_and_name_it = s_name_by_key.find(m_key);
    META_CHECK_ARG_DESCR(m_key, key_and_name_it != s_name_by_key.end(), "key name was not found");

    return m_modifiers == Modifiers::None
           ? key_and_name_it->second
           : fmt::format("{}{}{}", magic_enum::enum_name(m_modifiers), g_keys_separator, key_and_name_it->second);
};

State::State(std::initializer_list<Key> pressed_keys, Modifiers modifiers_mask)
    : m_modifiers_mask(modifiers_mask)
{
    META_FUNCTION_TASK();
    for (Key pressed_key : pressed_keys)
    {
        SetKeyImpl(pressed_key, KeyState::Pressed);
    }
}

bool State::operator<(const State& other) const noexcept
{
    META_FUNCTION_TASK();
    if (m_modifiers_mask != other.m_modifiers_mask)
    {
        return m_modifiers_mask < other.m_modifiers_mask;
    }
    return m_key_states < other.m_key_states;
}

bool State::operator==(const State& other) const noexcept
{
    META_FUNCTION_TASK();
    return m_key_states == other.m_key_states &&
           m_modifiers_mask == other.m_modifiers_mask;
}

State::operator bool() const noexcept
{
    META_FUNCTION_TASK();
    static const State s_empty_state;
    return operator!=(s_empty_state);
}

State::Properties State::GetDiff(const State& other) const noexcept
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    State::Properties properties_diff_mask = State::Properties::None;

    if (m_key_states != other.m_key_states)
        properties_diff_mask |= State::Properties::KeyStates;

    if (m_modifiers_mask != other.m_modifiers_mask)
        properties_diff_mask |= State::Properties::Modifiers;

    return properties_diff_mask;
}

KeyType State::SetKey(Key key, KeyState key_state)
{
    return SetKeyImpl(key, key_state);
}

KeyType State::SetKeyImpl(Key key, KeyState key_state)
{
    META_FUNCTION_TASK();
    if (key == Key::Unknown)
        return KeyType::Common;

    const Modifiers key_modifier = KeyConverter(key).GetModifierKey();
    if (key_modifier != Modifiers::None)
    {
        UpdateModifiersMask(key_modifier, key_state == KeyState::Pressed);
        return KeyType::Modifier;
    }

    const auto key_index = static_cast<size_t>(key);
    META_CHECK_ARG_LESS(key_index, m_key_states.size());
    m_key_states[key_index] = key_state;
    return KeyType::Common;
}

void State::UpdateModifiersMask(Modifiers modifier, bool add_modifier) noexcept
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    if (add_modifier)
        m_modifiers_mask |= modifier;
    else
        m_modifiers_mask &= ~modifier;
}

Keys State::GetPressedKeys() const noexcept
{
    META_FUNCTION_TASK();
    Keys pressed_keys;
    for (size_t key_index = 0; key_index < m_key_states.size(); ++key_index)
    {
        if (m_key_states[key_index] != KeyState::Pressed)
            continue;

        const auto key = static_cast<Key>(key_index);
        pressed_keys.insert(key);
    }
    return pressed_keys;
}

StateExt::StateExt(std::initializer_list<Key> pressed_keys, Modifiers modifiers_mask)
    : State(pressed_keys, modifiers_mask)
{
    META_FUNCTION_TASK();
    for (Key pressed_key : pressed_keys)
    {
        if (KeyConverter(pressed_key).GetModifierKey() != Modifiers::None)
            SetModifierKey(pressed_key, KeyState::Pressed);
    }
}

KeyType StateExt::SetKey(Key key, KeyState key_state)
{
    META_FUNCTION_TASK();
    const KeyType key_type = State::SetKey(key, key_state);
    if (key_type != KeyType::Modifier)
        return key_type;

    SetModifierKey(key, key_state);
    return KeyType::Modifier;
}

void StateExt::SetModifierKey(Key key, KeyState key_state)
{
    META_FUNCTION_TASK();
    if (key_state == KeyState::Pressed)
    {
        m_pressed_modifier_keys.insert(key);
    }
    else
    {
        m_pressed_modifier_keys.erase(key);
    }
}

Keys StateExt::GetAllPressedKeys() const
{
    META_FUNCTION_TASK();
    Keys all_pressed_keys = GetPressedKeys();
    all_pressed_keys.insert(m_pressed_modifier_keys.begin(), m_pressed_modifier_keys.end());
    return all_pressed_keys;
}

std::string State::ToString() const
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    const bool has_modifiers = m_modifiers_mask != Modifiers::None;
    if (has_modifiers)
    {
        ss << magic_enum::enum_name(m_modifiers_mask);
    }
    
    bool is_first_key = true;
    for (size_t key_index = 0; key_index < m_key_states.size(); ++key_index)
    {
        if (m_key_states[key_index] != KeyState::Pressed)
            continue;
        
        if (!is_first_key || has_modifiers)
        {
            ss << g_keys_separator;
        }
        
        const auto key = static_cast<Key>(key_index);
        ss << KeyConverter(key).ToString();
        is_first_key = false;
    }
    
    return ss.str();
}

} // namespace Methane::Platform::Keyboard
