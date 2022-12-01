/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

#include <Methane/Platform/Input/Keyboard.h>

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <map>
#include <sstream>

namespace Methane::Platform::Keyboard
{

static const std::string g_keys_separator       = "+";
static const std::string g_properties_separator = "+";

static ModifierMask GetModifierMask(Opt<Modifier> modifier_opt)
{
    META_FUNCTION_TASK();
    return modifier_opt ? ModifierMask(*modifier_opt) : ModifierMask();
}

KeyConverter::KeyConverter(Key key)
    : m_key(key)
    , m_modifiers(GetModifierMask(GetModifierKey()))
{
    META_FUNCTION_TASK();
}

KeyConverter::KeyConverter(Key key, ModifierMask modifiers)
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

Opt<Modifier> KeyConverter::GetModifierKey() const noexcept
{
    META_FUNCTION_TASK();
    switch (m_key)
    {
    case Key::LeftShift:
    case Key::RightShift:
        return Modifier::Shift;

    case Key::LeftControl:
    case Key::RightControl:
        return Modifier::Control;

    case Key::LeftAlt:
    case Key::RightAlt:
        return Modifier::Alt;

    case Key::LeftSuper:
    case Key::RightSuper:
        return Modifier::Super;

    case Key::CapsLock:
        return Modifier::CapsLock;
    case Key::NumLock:
        return Modifier::NumLock;

    default:
        return std::nullopt;
    }
}

std::string_view KeyConverter::GetKeyName() const
{
    META_FUNCTION_TASK();
    switch (m_key) // NOSONAR - long switch
    {
        // Control Keys
        case Key::LeftShift: return "LEFT_SHIFT";
        case Key::RightShift: return "RIGHT_SHIFT";
        case Key::LeftControl: return "LEFT_CONTROL";
        case Key::RightControl: return "RIGHT_CONTROL";
        case Key::LeftAlt: return "LEFT_ALT";
        case Key::RightAlt: return "RIGHT_ALT";
#ifdef __APPLE__
        case Key::LeftSuper: return "LEFT_COMMAND";
        case Key::RightSuper: return "RIGHT_COMMAND";
#else
        case Key::LeftSuper: return "LEFT_SUPER";
        case Key::RightSuper: return "RIGHT_SUPER";
#endif
        case Key::CapsLock: return "CAPSLOCK";
        case Key::ScrollLock: return "SCROLLOCK";
        case Key::NumLock: return "NUMLOCK";
        case Key::Menu: return "MENU";

        // Printable keys
        case Key::Space: return "SPACE";
        case Key::Apostrophe: return "'";
        case Key::Comma: return ",";
        case Key::Minus: return "-";
        case Key::Period: return ".";
        case Key::Slash: return "/";
        case Key::Num0: return "0";
        case Key::Num1: return "1";
        case Key::Num2: return "2";
        case Key::Num3: return "3";
        case Key::Num4: return "4";
        case Key::Num5: return "5";
        case Key::Num6: return "6";
        case Key::Num7: return "7";
        case Key::Num8: return "8";
        case Key::Num9: return "9";
        case Key::Semicolon: return ";";
        case Key::Equal: return "=";
        case Key::A: return "A";
        case Key::B: return "B";
        case Key::C: return "C";
        case Key::D: return "D";
        case Key::E: return "E";
        case Key::F: return "F";
        case Key::G: return "G";
        case Key::H: return "H";
        case Key::I: return "I";
        case Key::J: return "J";
        case Key::K: return "K";
        case Key::L: return "L";
        case Key::M: return "M";
        case Key::N: return "N";
        case Key::O: return "O";
        case Key::P: return "P";
        case Key::Q: return "Q";
        case Key::R: return "R";
        case Key::S: return "S";
        case Key::T: return "T";
        case Key::U: return "U";
        case Key::V: return "V";
        case Key::W: return "W";
        case Key::X: return "X";
        case Key::Y: return "Y";
        case Key::Z: return "Z";
        case Key::LeftBracket: return "[";
        case Key::BackSlash: return "\\";
        case Key::RightBracket: return "]";
        case Key::GraveAccent: return "`";
        case Key::World1: return "W1";
        case Key::World2: return "W2";

        // Function Keys
        case Key::Escape: return "ESC";
        case Key::Enter: return "ENTER";
        case Key::Tab: return "TAB";
        case Key::Backspace: return "BACKSPACE";
        case Key::Insert: return "INSERT";
        case Key::Delete: return "DELETE";
        case Key::Right: return "RIGHT";
        case Key::Left: return "LEFT";
        case Key::Down: return "DOWN";
        case Key::Up: return "UP";
        case Key::PageUp: return "PAGEUP";
        case Key::PageDown: return "PAGEDOWN";
        case Key::Home: return "HOME";
        case Key::End: return "END";
        case Key::PrintScreen: return "PRINTSCREEN";
        case Key::Pause: return "PAUSE";
        case Key::F1: return "F1";
        case Key::F2: return "F2";
        case Key::F3: return "F3";
        case Key::F4: return "F4";
        case Key::F5: return "F5";
        case Key::F6: return "F6";
        case Key::F7: return "F7";
        case Key::F8: return "F8";
        case Key::F9: return "F9";
        case Key::F10: return "F10";
        case Key::F11: return "F11";
        case Key::F12: return "F12";
        case Key::F13: return "F13";
        case Key::F14: return "F14";
        case Key::F15: return "F15";
        case Key::F16: return "F16";
        case Key::F17: return "F17";
        case Key::F18: return "F18";
        case Key::F19: return "F19";
        case Key::F20: return "F20";
        case Key::F21: return "F21";
        case Key::F22: return "F22";
        case Key::F23: return "F23";
        case Key::F24: return "F24";
        case Key::F25: return "F25";
        case Key::KeyPad0: return "KP0";
        case Key::KeyPad1: return "KP1";
        case Key::KeyPad2: return "KP2";
        case Key::KeyPad3: return "KP3";
        case Key::KeyPad4: return "KP4";
        case Key::KeyPad5: return "KP5";
        case Key::KeyPad6: return "KP6";
        case Key::KeyPad7: return "KP7";
        case Key::KeyPad8: return "KP8";
        case Key::KeyPad9: return "KP9";
        case Key::KeyPadDecimal: return "KP.";
        case Key::KeyPadDivide: return "KP/";
        case Key::KeyPadMultiply: return "KP*";
        case Key::KeyPadSubtract: return "KP-";
        case Key::KeyPadAdd: return "KP+";
        case Key::KeyPadEnter: return "KP-ENTER";
        case Key::KeyPadEqual: return "KP=";

        case Key::Unknown: return "Unknown";
        default:
            META_UNEXPECTED_ARG_DESCR_RETURN(m_key, "", "unexpected key value");
    }
}

std::string KeyConverter::ToString() const
{
    META_FUNCTION_TASK();
    return m_modifiers == ModifierMask{}
           ? std::string(GetKeyName())
           : fmt::format("{}{}{}", Data::GetEnumMaskName(m_modifiers), g_keys_separator, GetKeyName());
};

State::State(std::initializer_list<Key> pressed_keys, ModifierMask modifiers_mask)
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

State::PropertyMask State::GetDiff(const State& other) const noexcept
{
    META_FUNCTION_TASK();
    State::PropertyMask properties_diff_mask;

    if (m_key_states != other.m_key_states)
        properties_diff_mask.SetBitOn(State::Property::KeyStates);

    if (m_modifiers_mask != other.m_modifiers_mask)
        properties_diff_mask.SetBitOn(State::Property::Modifiers);

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

    if (const Opt<Modifier> key_modifier_opt = KeyConverter(key).GetModifierKey();
        key_modifier_opt)
    {
        UpdateModifiersMask(ModifierMask(*key_modifier_opt), key_state == KeyState::Pressed);
        return KeyType::Modifier;
    }

    const auto key_index = static_cast<size_t>(key);
    META_CHECK_ARG_LESS(key_index, m_key_states.size());
    m_key_states[key_index] = key_state;
    return KeyType::Common;
}

void State::UpdateModifiersMask(ModifierMask modifier, bool add_modifier) noexcept
{
    META_FUNCTION_TASK();
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

StateExt::StateExt(std::initializer_list<Key> pressed_keys, ModifierMask modifiers_mask)
    : State(pressed_keys, modifiers_mask)
{
    META_FUNCTION_TASK();
    for (Key pressed_key : pressed_keys)
    {
        if (KeyConverter(pressed_key).GetModifierKey().has_value())
            SetModifierKey(pressed_key, KeyState::Pressed);
    }
}

KeyType StateExt::SetKey(Key key, KeyState key_state)
{
    META_FUNCTION_TASK();
    if (const KeyType key_type = State::SetKey(key, key_state);
        key_type != KeyType::Modifier)
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
    bool is_first_key = true;

    if (m_modifiers_mask != ModifierMask{})
    {
        // Serialize modifiers
        Data::ForEachBitInEnumMask(m_modifiers_mask, [&ss, &is_first_key](Modifier modifier)
        {
            if (!is_first_key)
                ss << g_keys_separator;

            ss << magic_enum::enum_name(modifier);
            is_first_key = false;
        });
    }

    // Serialize regular keys
    for (size_t key_index = 0; key_index < m_key_states.size(); ++key_index)
    {
        if (m_key_states[key_index] != KeyState::Pressed)
            continue;
        
        if (!is_first_key)
            ss << g_keys_separator;
        
        const auto key = static_cast<Key>(key_index);
        ss << KeyConverter(key).ToString();
        is_first_key = false;
    }
    
    return ss.str();
}

} // namespace Methane::Platform::Keyboard
