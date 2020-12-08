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

FILE: Methane/Platform/Keyboard.h
Platform abstraction of keyboard events.

******************************************************************************/

#pragma once

#if defined _WIN32

#include "Windows/Keyboard.h"

#else

#include "Unix/Keyboard.h"

#endif

#include <magic_enum.hpp>
#include <array>
#include <set>
#include <string>
#include <ostream>

namespace Methane::Platform::Keyboard
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

    // Always keep at the end
    Unknown
};

namespace OS
{
#ifdef __APPLE__

constexpr Key g_key_left_ctrl  = Key::LeftSuper;
constexpr Key g_key_right_ctrl = Key::RightSuper;

#else

constexpr Key g_key_left_ctrl  = Key::LeftControl;
constexpr Key g_key_right_ctrl = Key::RightControl;

#endif
}

using Keys = std::set<Key>;

enum class Modifiers : uint32_t
{
    None     = 0U,
    Shift    = 1U << 0U,
    Control  = 1U << 1U,
    Alt      = 1U << 2U,
    Super    = 1U << 3U,
    CapsLock = 1U << 4U,
    NumLock  = 1U << 5U,
    All      = ~0U
};

enum class KeyType : uint32_t
{
    Common = 0U,
    Modifier,
};

struct NativeKey;

class KeyConverter
{
public:
    explicit KeyConverter(Key key);
    KeyConverter(Key key, Modifiers modifiers);
    explicit KeyConverter(const NativeKey& native_key);
    
    Key         GetKey() const noexcept         { return m_key; }
    Modifiers   GetModifiers() const noexcept   { return m_modifiers; }
    Modifiers   GetModifierKey() const noexcept;
    std::string ToString() const;
    
    // NOTE: Platform dependent functions: see MacOS, Windows subdirs for implementation
    static Key       GetKeyByNativeCode(const NativeKey& native_key);
    static Modifiers GetModifiersByNativeCode(const NativeKey& native_key);
    
private:
    const Key       m_key;
    const Modifiers m_modifiers;
    static Key GetControlKey(const NativeKey& native_key);
};

enum class KeyState : uint8_t
{
    Released = 0U,
    Pressed,
};

using KeyStates = std::array<KeyState, magic_enum::enum_count<Key>() - 1>;

class State
{
public:
    enum class Properties : uint32_t
    {
        None      = 0U,
        KeyStates = 1U << 0U,
        Modifiers = 1U << 1U,
        All       = ~0U
    };

    State() = default;
    State(std::initializer_list<Key> pressed_keys, Modifiers modifiers_mask = Modifiers::None);

    bool   operator<(const State& other) const noexcept;
    bool   operator==(const State& other) const noexcept;
    bool   operator!=(const State& other) const noexcept    { return !operator==(other); }
    const  KeyState& operator[](Key key) const noexcept     { return m_key_states[static_cast<size_t>(key)]; }
    explicit operator std::string() const                   { return ToString(); }
    explicit operator bool() const noexcept;

    virtual KeyType SetKey(Key key, KeyState key_state);

    void    SetModifiersMask(Modifiers mask) noexcept  { m_modifiers_mask = mask; }
    void    PressKey(Key key)                               { SetKey(key, KeyState::Pressed); }
    void    ReleaseKey(Key key)                             { SetKey(key, KeyState::Released); }

    Keys             GetPressedKeys() const noexcept;
    const KeyStates& GetKeyStates() const noexcept          { return m_key_states; }
    Modifiers        GetModifiersMask() const noexcept      { return m_modifiers_mask; }
    Properties       GetDiff(const State& other) const noexcept;
    std::string      ToString() const;

private:
    KeyType SetKeyImpl(Key key, KeyState key_state);
    void UpdateModifiersMask(Modifiers modifier_value, bool add_modifier) noexcept;

    KeyStates m_key_states{};
    Modifiers m_modifiers_mask = Modifiers::None;
};

inline std::ostream& operator<<(std::ostream& os, State const& keyboard_state)
{
    os << keyboard_state.ToString();
    return os;
}

// State tracks only active modifiers, but not exactly modifier keys
// This state extension tracks exactly what modifier keys are pressed
class StateExt : public State
{
public:
    StateExt() = default;
    StateExt(std::initializer_list<Key> pressed_keys, Modifiers modifiers_mask = Modifiers::None);

    KeyType SetKey(Key key, KeyState key_state) override;

    const Keys& GetPressedModifierKeys() const { return m_pressed_modifier_keys; }
    Keys        GetAllPressedKeys() const;

private:
    void SetModifierKey(Key key, KeyState key_state);

    Keys m_pressed_modifier_keys;
};

struct StateChange
{
    StateChange(const State& in_current, const State& in_previous, State::Properties in_changed_properties)
        : current(in_current)
        , previous(in_previous)
        , changed_properties(in_changed_properties)
    { }

    const State& current;
    const State& previous;
    const State::Properties changed_properties;
};

} // namespace Methane::Platform::Keyboard
