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

FILE: Methane/Platform/Keyboard.cpp
Platform abstraction of keyboard events.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>
#include <Methane/Instrumentation.h>

#include <map>
#include <sstream>
#include <cassert>

namespace Methane::Platform::Keyboard
{

static const std::string g_keys_separator = "+";
static const std::string g_properties_separator = "+";

KeyConverter::KeyConverter(Key key)
    : m_key(key)
    , m_modifiers(GetModifierKey())
{
    ITT_FUNCTION_TASK();
}

KeyConverter::KeyConverter(Key key, Modifier::Mask modifiers)
    : m_key(key)
    , m_modifiers(modifiers)
{
    ITT_FUNCTION_TASK();
}

KeyConverter::KeyConverter(const NativeKey& native_key)
    : m_key(GetKeyByNativeCode(native_key))
    , m_modifiers(GetModifiersByNativeCode(native_key))
{
    ITT_FUNCTION_TASK();
}

Modifier::Value KeyConverter::GetModifierKey() const noexcept
{
    ITT_FUNCTION_TASK();
    switch (m_key)
    {
    case Key::LeftShift:
    case Key::RightShift:   return Modifier::Shift;

    case Key::LeftControl:
    case Key::RightControl: return Modifier::Control;

    case Key::LeftAlt:
    case Key::RightAlt:     return Modifier::Alt;

    case Key::LeftSuper:
    case Key::RightSuper:   return Modifier::Super;

    case Key::CapsLock:     return Modifier::CapsLock;
    case Key::NumLock:      return Modifier::NumLock;

    default:                return Modifier::None;
    }
}

std::string KeyConverter::ToString() const noexcept
{
    ITT_FUNCTION_TASK();
    static const std::map<Key, std::string> s_name_by_key =
    {
        // Printable keys
        { Key::Space,           "SPACE"         },
        { Key::Apostrophe,      "'"             },
        { Key::Comma,           ","             },
        { Key::Minus,           "-"             },
        { Key::Period,          "."             },
        { Key::Slash,           "/"             },
        { Key::Num0,            "0"             },
        { Key::Num1,            "1"             },
        { Key::Num2,            "2"             },
        { Key::Num3,            "3"             },
        { Key::Num4,            "4"             },
        { Key::Num5,            "5"             },
        { Key::Num6,            "6"             },
        { Key::Num7,            "7"             },
        { Key::Num8,            "8"             },
        { Key::Num9,            "9"             },
        { Key::Semicolon,       ";"             },
        { Key::Equal,           "="             },
        { Key::A,               "A"             },
        { Key::B,               "B"             },
        { Key::C,               "C"             },
        { Key::D,               "D"             },
        { Key::E,               "E"             },
        { Key::F,               "F"             },
        { Key::G,               "G"             },
        { Key::H,               "H"             },
        { Key::I,               "I"             },
        { Key::J,               "J"             },
        { Key::K,               "K"             },
        { Key::L,               "L"             },
        { Key::M,               "M"             },
        { Key::N,               "N"             },
        { Key::O,               "O"             },
        { Key::P,               "P"             },
        { Key::Q,               "Q"             },
        { Key::R,               "R"             },
        { Key::S,               "S"             },
        { Key::T,               "T"             },
        { Key::U,               "U"             },
        { Key::V,               "V"             },
        { Key::W,               "W"             },
        { Key::X,               "X"             },
        { Key::Y,               "Y"             },
        { Key::Z,               "Z"             },
        { Key::LeftBracket,     "["             },
        { Key::BackSlash,       "\\"            },
        { Key::RightBracket,    "]"             },
        { Key::GraveAccent,     "`"             },
        { Key::World1,          "W1"            },
        { Key::World2,          "W2"            },
        
        // Function Keys
        { Key::Escape,          "ESC"           },
        { Key::Enter,           "ENTER"         },
        { Key::Tab,             "TAB"           },
        { Key::Backspace,       "BACKSPACE"     },
        { Key::Insert,          "INSERT"        },
        { Key::Delete,          "DELETE"        },
        { Key::Right,           "RIGHT"         },
        { Key::Left,            "LEFT"          },
        { Key::Down,            "DOWN"          },
        { Key::Up,              "UP"            },
        { Key::PageUp,          "PAGEUP"        },
        { Key::PageDown,        "PAGEDOWN"      },
        { Key::Home,            "HOME"          },
        { Key::End,             "END"           },
        { Key::CapsLock,        "CAPSLOCK"      },
        { Key::ScrollLock,      "SCROLLOCK"     },
        { Key::NumLock,         "NUMLOCK"       },
        { Key::PrintScreen,     "PRINTSCREEN"   },
        { Key::Pause,           "PAUSE"         },
        { Key::F1,              "F1"            },
        { Key::F2,              "F2"            },
        { Key::F3,              "F3"            },
        { Key::F4,              "F4"            },
        { Key::F5,              "F5"            },
        { Key::F6,              "F6"            },
        { Key::F7,              "F7"            },
        { Key::F8,              "F8"            },
        { Key::F9,              "F9"            },
        { Key::F10,             "F10"           },
        { Key::F11,             "F11"           },
        { Key::F12,             "F12"           },
        { Key::F13,             "F13"           },
        { Key::F14,             "F14"           },
        { Key::F15,             "F15"           },
        { Key::F16,             "F16"           },
        { Key::F17,             "F17"           },
        { Key::F18,             "F18"           },
        { Key::F19,             "F19"           },
        { Key::F20,             "F20"           },
        { Key::F21,             "F21"           },
        { Key::F22,             "F22"           },
        { Key::F23,             "F23"           },
        { Key::F24,             "F24"           },
        { Key::F25,             "F25"           },
        { Key::KeyPad0,         "KEYPAD0"       },
        { Key::KeyPad1,         "KEYPAD0"       },
        { Key::KeyPad2,         "KEYPAD0"       },
        { Key::KeyPad3,         "KEYPAD0"       },
        { Key::KeyPad4,         "KEYPAD0"       },
        { Key::KeyPad5,         "KEYPAD0"       },
        { Key::KeyPad6,         "KEYPAD0"       },
        { Key::KeyPad7,         "KEYPAD0"       },
        { Key::KeyPad8,         "KEYPAD0"       },
        { Key::KeyPad9,         "KEYPAD0"       },
        { Key::KeyPadDecimal,   "KEYPAD."       },
        { Key::KeyPadDivide,    "KEYPAD/"       },
        { Key::KeyPadMultiply,  "KEYPAD*"       },
        { Key::KeyPadSubtract,  "KEYPAD-"       },
        { Key::KeyPadAdd,       "KEYPAD+"       },
        { Key::KeyPadEnter,     "KEYPAD_ENTER"  },
        { Key::KeyPadEqual,     "KEYPAD="       },
        
        // Control keys
        { Key::LeftShift,       "LEFT_SHIFT"    },
        { Key::RightShift,      "RIGHT_SHIFT"   },
        { Key::LeftControl,     "LEFT_CONTROL"  },
        { Key::RightControl,    "RIGHT_CONTROL" },
        { Key::LeftAlt,         "LEFT_ALT"      },
        { Key::RightAlt,        "RIGHT_ALT"     },
#ifdef __APPLE__
        { Key::LeftSuper,       "LEFT_COMMAND"  },
        { Key::RightSuper,      "RIGHT_COMMAND" },
#else
        { Key::LeftSuper,       "LEFT_SUPER"    },
        { Key::RightSuper,      "RIGHT_SUPER"   },
#endif
        { Key::Menu,            "MENU"          }
    };

    auto key_and_name_it = s_name_by_key.find(m_key);
    if (key_and_name_it == s_name_by_key.end())
    {
        assert(0);
        return "";
    }
    
    return m_modifiers == Modifier::Value::None
         ? key_and_name_it->second
         : Modifier::ToString(m_modifiers) + g_keys_separator + key_and_name_it->second;
};

State::State(std::initializer_list<Key> pressed_keys, Modifier::Mask modifiers_mask)
    : m_modifiers_mask(modifiers_mask)
{
    ITT_FUNCTION_TASK();
    for (Key pressed_key : pressed_keys)
    {
        SetKey(pressed_key, KeyState::Pressed);
    }
}

State::State(const State& other)
    : m_key_states(other.m_key_states)
    , m_modifiers_mask(other.m_modifiers_mask)
{
    ITT_FUNCTION_TASK();
}

State& State::operator=(const State& other)
{
    ITT_FUNCTION_TASK();
    if (this != &other)
    {
        m_key_states     = other.m_key_states;
        m_modifiers_mask = other.m_modifiers_mask;
    }
    return *this;
}

bool State::operator<(const State& other) const
{
    ITT_FUNCTION_TASK();
    if (m_modifiers_mask != other.m_modifiers_mask)
    {
        return m_modifiers_mask < other.m_modifiers_mask;
    }
    return m_key_states < other.m_key_states;
}

bool State::operator==(const State& other) const
{
    ITT_FUNCTION_TASK();
    return m_key_states     == other.m_key_states &&
           m_modifiers_mask == other.m_modifiers_mask;
}

State::Property::Mask State::GetDiff(const State& other) const
{
    ITT_FUNCTION_TASK();
    State::Property::Mask properties_diff_mask = State::Property::None;

    if (m_key_states != other.m_key_states)
        properties_diff_mask |= State::Property::KeyStates;

    if (m_modifiers_mask != other.m_modifiers_mask)
        properties_diff_mask |= State::Property::Modifiers;

    return properties_diff_mask;
}

KeyType State::SetKey(Key key, KeyState state)
{
    ITT_FUNCTION_TASK();
    if (key == Key::Unknown)
        return KeyType::Common;

    const Modifier::Value key_modifier = KeyConverter(key).GetModifierKey();
    if (key_modifier != Modifier::Value::None)
    {
        UpdateModifiersMask(key_modifier, state == KeyState::Pressed);
        return KeyType::Common;
    }
    else
    {
        const size_t key_index = static_cast<size_t>(key);
        assert(key_index < m_key_states.size());
        if (key_index < m_key_states.size())
        {
            m_key_states[key_index] = state;
        }
        return KeyType::Modifier;
    }
}

void State::UpdateModifiersMask(Modifier::Value modifier, bool add_modifier)
{
    ITT_FUNCTION_TASK();
    if (add_modifier)
        m_modifiers_mask |= modifier;
    else
        m_modifiers_mask &= ~modifier;
}

Keys State::GetPressedKeys() const
{
    ITT_FUNCTION_TASK();
    Keys pressed_keys;
    for (size_t key_index = 0; key_index < m_key_states.size(); ++key_index)
    {
        if (m_key_states[key_index] != KeyState::Pressed)
            continue;

        const Key key = static_cast<Key>(key_index);
        pressed_keys.insert(key);
    }
    return pressed_keys;
}

KeyType StateExt::SetKey(Key key, KeyState state)
{
    ITT_FUNCTION_TASK();
    const KeyType key_type = State::SetKey(key, state);
    if (key_type == KeyType::Modifier)
    {
        if (state == KeyState::Pressed)
        {
            m_pressed_modifier_keys.insert(key);
        }
        else
        {
            m_pressed_modifier_keys.erase(key);
        }
    }
    return key_type;
}

Keys StateExt::GetAllPressedKeys() const
{
    ITT_FUNCTION_TASK();
    Keys all_pressed_keys = GetPressedKeys();
    all_pressed_keys.insert(m_pressed_modifier_keys.begin(), m_pressed_modifier_keys.end());
    return all_pressed_keys;
}

std::string Modifier::ToString(Value modifier)
{
    ITT_FUNCTION_TASK();
    switch(modifier)
    {
        case None:      return "None";
        case Shift:     return "Shift";
        case Control:   return "Control";
        case Alt:       return "Alt";
        case CapsLock:  return "CapsLock";
        case NumLock:   return "NumLock";
        case All:       return "All";
#ifdef __APPLE__
        case Super:     return "Command";
#else
        case Super:     return "Super";
#endif
    }
    return "Undefined";
}

std::string Modifier::ToString(Modifier::Mask modifiers_mask)
{
    ITT_FUNCTION_TASK();
    std::stringstream ss;
    bool first_modifier = true;
    for(Value modifier : values)
    {
        if (modifiers_mask & modifier)
        {
            if (!first_modifier)
            {
                ss << g_keys_separator;
            }
            ss << ToString(modifier);
            first_modifier = false;
        }
    }
    return ss.str();
}

std::string State::Property::ToString(State::Property::Value property_value)
{
    ITT_FUNCTION_TASK();
    switch (property_value)
    {
    case All:       return "All";
    case KeyStates: return "KeyStates";
    case Modifiers: return "Modifiers";
    case None:      return "None";
    }
    return "Undefined";
}

std::string State::Property::ToString(State::Property::Mask properties_mask)
{
    ITT_FUNCTION_TASK();
    std::stringstream ss;
    bool first_property = true;
    for (Value property_value : values)
    {
        if (!(properties_mask & property_value))
            continue;

        if (!first_property)
        {
            ss << g_properties_separator;
        }
        ss << ToString(property_value);
        first_property = false;
    }
    return ss.str();
}

std::string State::ToString() const
{
    ITT_FUNCTION_TASK();
    std::stringstream ss;
    const std::string modifiers_str = Modifier::ToString(m_modifiers_mask);
    if (!modifiers_str.empty())
    {
        ss << modifiers_str;
    }
    
    bool is_first_key = true;
    for (size_t key_index = 0; key_index < m_key_states.size(); ++key_index)
    {
        if (m_key_states[key_index] != KeyState::Pressed)
            continue;
        
        if (!is_first_key || !modifiers_str.empty())
        {
            ss << g_keys_separator;
        }
        
        const Key key = static_cast<Key>(key_index);
        ss << KeyConverter(key).ToString();
        is_first_key = false;
    }
    
    return ss.str();
}

} // namespace Methane::Platform::Keyboard
