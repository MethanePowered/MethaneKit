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

 FILE: Methane/Platform/Input/State.cpp
 Aggregated application input state with controllers.

******************************************************************************/

#include <Methane/Platform/Input/State.h>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Platform::Input
{

void State::OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state)
{
    ITT_FUNCTION_TASK();

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetButton(button, button_state);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseButtonChanged(button, button_state, Mouse::StateChange(m_mouse_state, prev_mouse_state, Mouse::State::Property::Buttons));
}

void State::OnMousePositionChanged(const Mouse::Position& mouse_position)
{
    ITT_FUNCTION_TASK();

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetPosition(mouse_position);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMousePositionChanged(mouse_position, Mouse::StateChange(m_mouse_state, prev_mouse_state, Mouse::State::Property::Position));
}

void State::OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta)
{
    ITT_FUNCTION_TASK();

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.AddScrollDelta(mouse_scroll_delta);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseScrollChanged(mouse_scroll_delta, Mouse::StateChange(m_mouse_state, prev_mouse_state, Mouse::State::Property::Scroll));
}

void State::OnMouseInWindowChanged(bool is_mouse_in_window)
{
    ITT_FUNCTION_TASK();

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetInWindow(is_mouse_in_window);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseInWindowChanged(is_mouse_in_window, Mouse::StateChange(m_mouse_state, prev_mouse_state, Mouse::State::Property::InWindow));
}

void State::OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state)
{
    ITT_FUNCTION_TASK();

    Keyboard::State prev_keyboard_state(m_keyboard_state);
    m_keyboard_state.SetKey(key, key_state);
    Keyboard::State::Property::Mask state_changes_mask = m_keyboard_state.GetDiff(prev_keyboard_state);

    if (state_changes_mask == Keyboard::State::Property::None)
        return;

    m_controllers.OnKeyboardChanged(key, key_state, Keyboard::StateChange(m_keyboard_state, prev_keyboard_state, state_changes_mask));
}

void State::OnModifiersChanged(Keyboard::Modifier::Mask modifiers_mask)
{
    ITT_FUNCTION_TASK();

    Keyboard::State prev_keyboard_state(m_keyboard_state);
    m_keyboard_state.SetModifiersMask(modifiers_mask);
    Keyboard::State::Property::Mask state_changes_mask = m_keyboard_state.GetDiff(prev_keyboard_state);
    
    if (state_changes_mask == Keyboard::State::Property::None)
        return;
    
    m_controllers.OnModifiersChanged(modifiers_mask, Keyboard::StateChange(m_keyboard_state, prev_keyboard_state, state_changes_mask));
}

void State::ReleaseAllKeys()
{
    ITT_FUNCTION_TASK();
    Keyboard::Keys pressed_keys = m_keyboard_state.GetAllPressedKeys();
    for (Keyboard::Key key : pressed_keys)
    {
        OnKeyboardChanged(key, Keyboard::KeyState::Released);
    }
}

} // namespace Methane::Platform::Input
