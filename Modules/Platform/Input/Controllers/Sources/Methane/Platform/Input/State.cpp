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

 FILE: Methane/Platform/Input/State.cpp
 Aggregated application input state with controllers.

******************************************************************************/

#include <Methane/Platform/Input/State.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Platform::Input
{

State::State(const ControllersPool& controllers)
    : m_controllers(controllers)
{ }

const ControllersPool& State::GetControllers() const noexcept
{
    return m_controllers;
}

void State::AddControllers(const Ptrs<Controller>& controllers)
{
    META_FUNCTION_TASK();
    m_controllers.insert(m_controllers.end(), controllers.begin(), controllers.end());
}

const Input::Keyboard::State& State::GetKeyboardState() const noexcept
{
    return m_keyboard_state;
}

const Input::Mouse::State& State::GetMouseState() const noexcept
{
    return m_mouse_state;
}

void State::OnMouseButtonChanged(Mouse::Button button, Input::Mouse::ButtonState button_state)
{
    META_FUNCTION_TASK();

    Input::Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetButton(button, button_state);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseButtonChanged(button, button_state,
                                       Input::Mouse::StateChange(m_mouse_state, prev_mouse_state,
                                                          Input::Mouse::State::PropertyMask(Mouse::State::Property::Buttons)));
}

void State::OnMousePositionChanged(const Input::Mouse::Position& mouse_position)
{
    META_FUNCTION_TASK();

    Input::Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetPosition(mouse_position);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMousePositionChanged(mouse_position,
                                         Input::Mouse::StateChange(m_mouse_state, prev_mouse_state,
                                                            Input::Mouse::State::PropertyMask(Mouse::State::Property::Position)));
}

void State::OnMouseScrollChanged(const Input::Mouse::Scroll& mouse_scroll_delta)
{
    META_FUNCTION_TASK();

    Input::Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.AddScrollDelta(mouse_scroll_delta);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseScrollChanged(mouse_scroll_delta,
                                       Input::Mouse::StateChange(m_mouse_state, prev_mouse_state,
                                                          Input::Mouse::State::PropertyMask(Mouse::State::Property::Scroll)));
}

void State::OnMouseInWindowChanged(bool is_mouse_in_window)
{
    META_FUNCTION_TASK();

    Input::Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetInWindow(is_mouse_in_window);

    if (m_mouse_state == prev_mouse_state)
        return;

    m_controllers.OnMouseInWindowChanged(is_mouse_in_window,
                                         Input::Mouse::StateChange(m_mouse_state, prev_mouse_state,
                                                            Input::Mouse::State::PropertyMask(Mouse::State::Property::InWindow)));
}

void State::OnKeyboardChanged(Keyboard::Key key, Input::Keyboard::KeyState key_state)
{
    META_FUNCTION_TASK();

    Input::Keyboard::State prev_keyboard_state(static_cast<const Input::Keyboard::State&>(m_keyboard_state));
    m_keyboard_state.SetKey(key, key_state);
    Input::Keyboard::State::PropertyMask state_changes_mask = m_keyboard_state.GetDiff(prev_keyboard_state);

    if (state_changes_mask == Input::Keyboard::State::PropertyMask{})
        return;

    m_controllers.OnKeyboardChanged(key, key_state, Input::Keyboard::StateChange(m_keyboard_state, prev_keyboard_state, state_changes_mask));
}

void State::OnModifiersChanged(Keyboard::ModifierMask modifiers_mask)
{
    META_FUNCTION_TASK();

    Input::Keyboard::State prev_keyboard_state(static_cast<const Input::Keyboard::State&>(m_keyboard_state));
    m_keyboard_state.SetModifiersMask(modifiers_mask);
    Input::Keyboard::State::PropertyMask state_changes_mask = m_keyboard_state.GetDiff(prev_keyboard_state);
    
    if (state_changes_mask == Input::Keyboard::State::PropertyMask{})
        return;
    
    m_controllers.OnModifiersChanged(modifiers_mask, Input::Keyboard::StateChange(m_keyboard_state, prev_keyboard_state, state_changes_mask));
}

void State::ReleaseAllKeys()
{
    META_FUNCTION_TASK();
    Input::Keyboard::Keys pressed_keys = m_keyboard_state.GetAllPressedKeys();
    for (Keyboard::Key key : pressed_keys)
    {
        OnKeyboardChanged(key, Input::Keyboard::KeyState::Released);
    }
}

} // namespace Methane::Platform::Input
