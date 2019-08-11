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

FILE: Methane/Platform/Keyboard.h
Platform abstraction of keyboard events.

******************************************************************************/

#pragma once

#if defined _WIN32

#include "Windows/KeyboardWin.h"

#elif defined __APPLE__

#include "MacOS/KeyboardMac.h"

#endif

#include <array>
#include <set>
#include <string>

namespace Methane
{
namespace Platform
{
namespace Keyboard
{

enum class Key : uint32_t
{
    // Printable keys
    Space           = 0,
    Apostrophe,     // '
    Comma,          // ,
    Minus,          // -
    Period,         // .
    Slash,          // /
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon,      // ;
    Equal,          // =
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket,    //  [
    BackSlash,      // '\'
    RightBracket,   //  ]
    GraveAccent,    //  `
    World1, World2,
    
    // Function Keys
    Escape, Enter, Tab,  Backspace, Insert, Delete,
    Right,  Left,  Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
    F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10,
    F11, F12, F13, F14, F15, F16, F17, F18, F19, F20,
    F21, F22, F23, F24, F25,
    KeyPad0, KeyPad1, KeyPad2, KeyPad3, KeyPad4,
    KeyPad5, KeyPad6, KeyPad7, KeyPad8, KeyPad9,
    KeyPadDecimal,  KeyPadDivide, KeyPadMultiply,
    KeyPadSubtract, KeyPadAdd,
    KeyPadEnter,    KeyPadEqual,
    
    // Control keys
    LeftShift,  LeftControl,  LeftAlt,  LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper,
    Menu,

    Count,
    Unknown
};

using Keys = std::set<Key>;

struct Modifier
{
    using Mask = uint32_t;
    enum Value : Mask
    {
        None     = 0,
        Shift    = 1 << 0,
        Control  = 1 << 1,
        Alt      = 1 << 2,
        Super    = 1 << 3,
        CapsLock = 1 << 4,
        NumLock  = 1 << 5,
        All      = static_cast<Mask>(~0),
    };

    using Values = std::array<Value, 6>;
    static constexpr const Values values = { Shift, Control, Alt, Super, CapsLock, NumLock };

    static std::string ToString(Value modifier);
    static std::string ToString(Mask modifiers_mask);

    Modifier() = delete;
    ~Modifier() = delete;
};

struct NativeKey;

class KeyConverter
{
public:
    KeyConverter(Key key);
    KeyConverter(Key key, Modifier::Mask modifiers);
    KeyConverter(const NativeKey& native_key);
    
    Key             GetKey() const noexcept         { return m_key; }
    Modifier::Mask  GetModifiers() const noexcept   { return m_modifiers; }
    Modifier::Value GetModifierKey() const noexcept;
    std::string     ToString() const noexcept;
    
    // NOTE: Platform dependent functions: see MacOS, Windows subdirs for implementation
    static Key            GetKeyByNativeCode(const NativeKey& native_key);
    static Modifier::Mask GetModifiersByNativeCode(const NativeKey& native_key);
    
private:
    const Key            m_key;
    const Modifier::Mask m_modifiers;
};

enum class KeyState : uint8_t
{
    Released = 0,
    Pressed,
};

using KeyStates = std::array<KeyState, static_cast<size_t>(Key::Count)>;

class State
{
public:
    struct Property
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            None      = 0,
            KeyStates = 1 << 0,
            Modifiers = 1 << 1,
            All       = static_cast<Mask>(~0),
        };

        using Values = std::array<Value, 2>;
        static constexpr const Values values = { KeyStates, Modifiers };

        static std::string ToString(Value modifier);
        static std::string ToString(Mask modifiers_mask);

        Property() = delete;
        ~Property() = delete;
    };

    State() = default;
    State(std::initializer_list<Key> pressed_keys, Modifier::Mask modifiers_mask = Modifier::Value::None);
    State(const State& other);

    State& operator=(const State& other);
    bool   operator<(const State& other) const;
    bool   operator==(const State& other) const;
    bool   operator!=(const State& other) const     { return !operator==(other); }
    const  KeyState& operator[](Key key) const      { return m_key_states[static_cast<size_t>(key)]; }
    operator std::string() const                    { return ToString(); }

    void  SetKey(Key key, KeyState state);
    void  SetModifiersMask(Modifier::Mask mask)     { m_modifiers_mask = mask; }
    void  PressKey(Key key)                         { SetKey(key, KeyState::Pressed); }
    void  ReleaseKey(Key key)                       { SetKey(key, KeyState::Released); }

    Keys                GetPressedKeys() const;
    const KeyStates&    GetKeyStates() const        { return m_key_states; }
    Modifier::Mask      GetModifiersMask() const    { return m_modifiers_mask; }
    Property::Mask      GetDiff(const State& other) const;
    std::string         ToString() const;

private:
    void UpdateModifiersMask(Modifier::Value modifier_value, bool add_modifier);

    KeyStates       m_key_states{};
    Modifier::Mask  m_modifiers_mask = Modifier::None;
};

inline std::ostream& operator<<( std::ostream& os, State const& keyboard_state)
{
    os << keyboard_state.ToString();
    return os;
}

struct StateChange
{
    StateChange(const State& in_current, const State& in_previous, State::Property::Mask in_changed_properties)
        : current(in_current)
        , previous(in_previous)
        , changed_properties(in_changed_properties)
    { }

    const State& current;
    const State& previous;
    const State::Property::Mask changed_properties;
};

} // namespace Keyboard
} // namespace Platform
} // namespace Methane
