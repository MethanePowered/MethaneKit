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

 FILE: Methane/Platform/Input/State.cpp
 Aggregated application input state with controllers.

******************************************************************************/

#include <Methane/Platform/Input/State.h>

#include <cassert>

using namespace Methane::Platform;
using namespace Methane::Platform::Input;

void State::KeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state)
{
    if (m_controllers.empty())
        return;

    Keyboard::State prev_keyboard_state(m_keyboard_state);
    m_keyboard_state.SetKey(key, key_state);
    Keyboard::State::Property::Mask state_changes_mask = m_keyboard_state.GetDiff(prev_keyboard_state);
    if (state_changes_mask == Keyboard::State::Property::All)
        return;

    m_controllers.OnKeyboardStateChanged(m_keyboard_state, prev_keyboard_state, state_changes_mask);
}

void State::MouseButtonsChanged(Mouse::Button button, Mouse::ButtonState button_state)
{
    if (m_controllers.empty())
        return;

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetButton(button, button_state);
    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseStateChanged(m_mouse_state, prev_mouse_state, Mouse::State::Property::Buttons);
}

void State::MousePositionChanged(Mouse::Position mouse_position)
{
    if (m_controllers.empty())
        return;

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetPosition(mouse_position);
    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseStateChanged(m_mouse_state, m_mouse_state, Mouse::State::Property::Position);
}

void State::MouseInWindowChanged(bool is_mouse_in_window)
{
    if (m_controllers.empty() || m_mouse_in_window == is_mouse_in_window)
        return;

    m_mouse_in_window = is_mouse_in_window;
    
    m_controllers.OnMouseStateChanged(m_mouse_state, m_mouse_state, Mouse::State::Property::None);
}
