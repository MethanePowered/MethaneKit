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

FILE: Methane/Platform/Keyboard.cpp
Platform abstraction of keyboard events.

******************************************************************************/

#include <Methane/Platform/Keyboard.h>

using namespace Methane::Platform::Keyboard;

State::State(std::initializer_list<Key> pressed_keys)
{
    for (Key pressed_key : pressed_keys)
    {
        SetKey(pressed_key, KeyState::Pressed);
    }
}

State::State(const State& other)
    : m_key_states(other.m_key_states)
    , m_modifiers_mask(other.m_modifiers_mask)
{
}

void State::SetKey(Key key, KeyState state)
{
    switch (key)
    {
    case Key::LeftShift:
    case Key::RightShift:
        return UpdateModifiersMask(Modifier::Shift,     state == KeyState::Pressed);
    case Key::LeftControl:
    case Key::RightControl:
        return UpdateModifiersMask(Modifier::Control,   state == KeyState::Pressed);
    case Key::LeftAlt:
    case Key::RightAlt:
        return UpdateModifiersMask(Modifier::Alt,       state == KeyState::Pressed);
    case Key::LeftSuper:
    case Key::RightSuper:
        return UpdateModifiersMask(Modifier::Super,     state == KeyState::Pressed);
    case Key::CapsLock:
        return UpdateModifiersMask(Modifier::CapsLock,  state == KeyState::Pressed);
    case Key::NumLock:
        return UpdateModifiersMask(Modifier::NumLock,   state == KeyState::Pressed);
    }
    
    m_key_states[static_cast<size_t>(key)] = state;
}

void State::UpdateModifiersMask(Modifier::Value modifier, bool add_modifier)
{
    if (add_modifier)
        m_modifiers_mask |= modifier;
    else
        m_modifiers_mask &= ~modifier;
}

Keys State::GetPressedKeys() const
{
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