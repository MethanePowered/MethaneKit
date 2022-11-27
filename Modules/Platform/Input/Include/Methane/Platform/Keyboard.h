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

FILE: Methane/Platform/Keyboard.h
Platform abstraction of keyboard events.

******************************************************************************/

#pragma once

#if defined _WIN32

#include "Windows/Keyboard.h"

#elif defined __APPLE__

#include "MacOS/Keyboard.h"

#else

#include "Linux/Keyboard.h"

#endif

#include <Methane/Data/EnumMask.hpp>
#include <Methane/Memory.hpp>

#include <array>
#include <set>
#include <string>
#include <string_view>
#include <ostream>

namespace Methane::Platform::Keyboard
{

enum class Key : uint32_t
{
    // Control keys
    LeftShift, RightShift,
    LeftControl, RightControl,
    LeftAlt, RightAlt,
    LeftSuper, RightSuper,
    CapsLock, ScrollLock, NumLock,
    Menu,

    // Printable keys
    Space,
    Semicolon,      // ; NOSONAR
    Apostrophe,     // ' "
    BackSlash,      // '\' |
    LeftBracket,    //  [ {
    RightBracket,   //  ] }
    Comma,          // , <
    Period,         // . >
    Slash,          // / ?
    Minus,          // - _
    Equal,          // = +
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    GraveAccent,    //  ` ~
    World1, World2,
    
    // Function Keys
    Escape, Enter, Tab,  Backspace, Insert, Delete,
    Right,  Left,  Down, Up,
    PageUp, PageDown, Home, End,
    PrintScreen, Pause,
    F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10,
    F11, F12, F13, F14, F15, F16, F17, F18, F19, F20,
    F21, F22, F23, F24, F25,
    KeyPad0, KeyPad1, KeyPad2, KeyPad3, KeyPad4,
    KeyPad5, KeyPad6, KeyPad7, KeyPad8, KeyPad9,
    KeyPadDecimal,  KeyPadDivide, KeyPadMultiply,
    KeyPadSubtract, KeyPadAdd,
    KeyPadEnter,    KeyPadEqual,

    // Always keep at the end
    Unknown,
    Count
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

enum class Modifier : uint32_t
{
    Alt,
    Control,
    Shift,
    Super,
    CapsLock,
    NumLock
};

using ModifierMask = Data::EnumMask<Modifier>;

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
    KeyConverter(Key key, ModifierMask modifiers);
    explicit KeyConverter(const NativeKey& native_key);
    
    [[nodiscard]] Key              GetKey() const noexcept         { return m_key; }
    [[nodiscard]] ModifierMask     GetModifiers() const noexcept   { return m_modifiers; }
    [[nodiscard]] Opt<Modifier>    GetModifierKey() const noexcept;
    [[nodiscard]] std::string_view GetKeyName() const;
    [[nodiscard]] std::string      ToString() const;
    
    // NOTE: Platform dependent functions: see MacOS, Windows subdirs for implementation
    [[nodiscard]] static Key           GetKeyByNativeCode(const NativeKey& native_key) noexcept;
    [[nodiscard]] static Opt<Modifier> GetModifiersByNativeCode(const NativeKey& native_key) noexcept;
    
private:
    [[nodiscard]] static Key GetControlKey(const NativeKey& native_key);

    const Key       m_key;
    const ModifierMask m_modifiers;
};

enum class KeyState : uint8_t
{
    Released,
    Pressed
};

using KeyStates = std::array<KeyState, static_cast<size_t>(Key::Count) - 1>;

class State
{
public:
    enum class Property : uint32_t
    {
        KeyStates,
        Modifiers,
    };

    using PropertyMask = Data::EnumMask<Property>;

    State() = default;
    State(std::initializer_list<Key> pressed_keys, ModifierMask modifiers_mask = {});
    virtual ~State() = default;

    [[nodiscard]] bool operator<(const State& other) const noexcept;
    [[nodiscard]] bool operator==(const State& other) const noexcept;
    [[nodiscard]] bool operator!=(const State& other) const noexcept  { return !operator==(other); }
    [[nodiscard]] const  KeyState& operator[](Key key) const noexcept { return m_key_states[static_cast<size_t>(key)]; }
    [[nodiscard]] explicit operator std::string() const               { return ToString(); }
    [[nodiscard]] explicit operator bool() const noexcept;

    virtual KeyType SetKey(Key key, KeyState key_state);

    void SetModifiersMask(ModifierMask mask) noexcept { m_modifiers_mask = mask; }
    void PressKey(Key key)                         { SetKey(key, KeyState::Pressed); }
    void ReleaseKey(Key key)                       { SetKey(key, KeyState::Released); }

    [[nodiscard]] Keys             GetPressedKeys() const noexcept;
    [[nodiscard]] const KeyStates& GetKeyStates() const noexcept     { return m_key_states; }
    [[nodiscard]] ModifierMask        GetModifiersMask() const noexcept { return m_modifiers_mask; }
    [[nodiscard]] PropertyMask     GetDiff(const State& other) const noexcept;
    [[nodiscard]] std::string      ToString() const;

private:
    KeyType SetKeyImpl(Key key, KeyState key_state);
    void UpdateModifiersMask(ModifierMask modifier_value, bool add_modifier) noexcept;

    KeyStates    m_key_states{};
    ModifierMask m_modifiers_mask;
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
    StateExt(std::initializer_list<Key> pressed_keys, ModifierMask modifiers_mask = {});

    KeyType SetKey(Key key, KeyState key_state) override;

    [[nodiscard]] const Keys& GetPressedModifierKeys() const { return m_pressed_modifier_keys; }
    [[nodiscard]] Keys        GetAllPressedKeys() const;

private:
    void SetModifierKey(Key key, KeyState key_state);

    Keys m_pressed_modifier_keys;
};

struct StateChange
{
    StateChange(const State& in_current, const State& in_previous, State::PropertyMask in_changed_properties)
        : current(in_current)
        , previous(in_previous)
        , changed_properties(in_changed_properties)
    { }

    const State& current;
    const State& previous;
    const State::PropertyMask changed_properties;
};

} // namespace Methane::Platform::Keyboard
