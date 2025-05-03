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
#include <Methane/Checks.hpp>

#include <fmt/format.h>
#include <map>
#include <sstream>

namespace Methane::Platform::Input::Keyboard
{

static const std::string g_keys_separator       = "+";

static ModifierMask GetModifierMask(Opt<Modifier> modifier_opt)
{
    META_FUNCTION_TASK();
    return modifier_opt ? ModifierMask(*modifier_opt) : ModifierMask();
}

KeyConverter::KeyConverter(Key key)
    : m_key(key)
    , m_modifiers(GetModifierMask(GetModifierKey()))
{ }

KeyConverter::KeyConverter(Key key, ModifierMask modifiers)
    : m_key(key)
    , m_modifiers(modifiers)
{ }

KeyConverter::KeyConverter(const NativeKey& native_key)
    : m_key(GetKeyByNativeCode(native_key))
    , m_modifiers(GetModifiersByNativeCode(native_key))
{ }

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
        using enum Key;
        // Control Keys
        case LeftShift: return "LEFT_SHIFT";
        case RightShift: return "RIGHT_SHIFT";
        case LeftControl: return "LEFT_CONTROL";
        case RightControl: return "RIGHT_CONTROL";
        case LeftAlt: return "LEFT_ALT";
        case RightAlt: return "RIGHT_ALT";
#ifdef __APPLE__
        case LeftSuper: return "LEFT_COMMAND";
        case RightSuper: return "RIGHT_COMMAND";
#else
        case LeftSuper: return "LEFT_SUPER";
        case RightSuper: return "RIGHT_SUPER";
#endif
        case CapsLock: return "CAPSLOCK";
        case ScrollLock: return "SCROLLOCK";
        case NumLock: return "NUMLOCK";
        case Menu: return "MENU";

        // Printable keys
        case Space: return "SPACE";
        case Apostrophe: return "'";
        case Comma: return ",";
        case Minus: return "-";
        case Period: return ".";
        case Slash: return "/";
        case Num0: return "0";
        case Num1: return "1";
        case Num2: return "2";
        case Num3: return "3";
        case Num4: return "4";
        case Num5: return "5";
        case Num6: return "6";
        case Num7: return "7";
        case Num8: return "8";
        case Num9: return "9";
        case Semicolon: return ";";
        case Equal: return "=";
        case A: return "A";
        case B: return "B";
        case C: return "C";
        case D: return "D";
        case E: return "E";
        case F: return "F";
        case G: return "G";
        case H: return "H";
        case I: return "I";
        case J: return "J";
        case K: return "K";
        case L: return "L";
        case M: return "M";
        case N: return "N";
        case O: return "O";
        case P: return "P";
        case Q: return "Q";
        case R: return "R";
        case S: return "S";
        case T: return "T";
        case U: return "U";
        case V: return "V";
        case W: return "W";
        case X: return "X";
        case Y: return "Y";
        case Z: return "Z";
        case LeftBracket: return "[";
        case BackSlash: return "\\";
        case RightBracket: return "]";
        case GraveAccent: return "`";
        case World1: return "W1";
        case World2: return "W2";

        // Function Keys
        case Escape: return "ESC";
        case Enter: return "ENTER";
        case Tab: return "TAB";
        case Backspace: return "BACKSPACE";
        case Insert: return "INSERT";
        case Delete: return "DELETE";
        case Right: return "RIGHT";
        case Left: return "LEFT";
        case Down: return "DOWN";
        case Up: return "UP";
        case PageUp: return "PAGEUP";
        case PageDown: return "PAGEDOWN";
        case Home: return "HOME";
        case End: return "END";
        case PrintScreen: return "PRINTSCREEN";
        case Pause: return "PAUSE";
        case F1: return "F1";
        case F2: return "F2";
        case F3: return "F3";
        case F4: return "F4";
        case F5: return "F5";
        case F6: return "F6";
        case F7: return "F7";
        case F8: return "F8";
        case F9: return "F9";
        case F10: return "F10";
        case F11: return "F11";
        case F12: return "F12";
        case F13: return "F13";
        case F14: return "F14";
        case F15: return "F15";
        case F16: return "F16";
        case F17: return "F17";
        case F18: return "F18";
        case F19: return "F19";
        case F20: return "F20";
        case F21: return "F21";
        case F22: return "F22";
        case F23: return "F23";
        case F24: return "F24";
        case F25: return "F25";
        case KeyPad0: return "KP0";
        case KeyPad1: return "KP1";
        case KeyPad2: return "KP2";
        case KeyPad3: return "KP3";
        case KeyPad4: return "KP4";
        case KeyPad5: return "KP5";
        case KeyPad6: return "KP6";
        case KeyPad7: return "KP7";
        case KeyPad8: return "KP8";
        case KeyPad9: return "KP9";
        case KeyPadDecimal: return "KP.";
        case KeyPadDivide: return "KP/";
        case KeyPadMultiply: return "KP*";
        case KeyPadSubtract: return "KP-";
        case KeyPadAdd: return "KP+";
        case KeyPadEnter: return "KP-ENTER";
        case KeyPadEqual: return "KP=";

        case Unknown: return "Unknown";
        default:
            META_UNEXPECTED_RETURN_DESCR(m_key, "", "unexpected key value");
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

State::operator bool() const noexcept
{
    META_FUNCTION_TASK();
    static const State s_empty_state;
    return *this != s_empty_state;
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
    META_CHECK_LESS(key_index, m_key_states.size());
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

} // namespace Methane::Platform::Input::Keyboard
