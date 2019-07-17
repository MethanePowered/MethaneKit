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

FILE: Methane/Platform/InputControllersPool.cpp
A pool of input controllers for user actions handling in a separated model components.

******************************************************************************/

#include <Methane/Platform/InputControllersPool.h>

#include <cassert>

// Uncomment define to print user input actions (keyboard, mouse) to debug output
//#define DEBUG_USER_INPUT

using namespace Methane::Platform;

void InputControllersPool::AddController(InputController::Ptr&& sp_controller)
{
    m_controllers.emplace_back(std::move(sp_controller));
}

void InputControllersPool::KeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state)
{
    if (m_controllers.empty())
        return;

    Keyboard::State prev_keyboard_state(m_keyboard_state);
    m_keyboard_state.SetKey(key, key_state);
    Keyboard::State::Property::Mask state_changes_mask = m_keyboard_state.GetDiff(prev_keyboard_state);
    if (state_changes_mask == Keyboard::State::Property::All)
        return;

    OnKeyboardStateChanged(m_keyboard_state, prev_keyboard_state, state_changes_mask);
}

void InputControllersPool::MouseButtonsChanged(Mouse::Button button, Mouse::ButtonState button_state)
{
    if (m_controllers.empty())
        return;

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetButton(button, button_state);
    if (m_mouse_state == prev_mouse_state)
        return;

    OnMouseStateChanged(m_mouse_state, prev_mouse_state, Mouse::State::Property::Buttons);
}

void InputControllersPool::MousePositionChanged(Mouse::Position mouse_position)
{
    if (m_controllers.empty())
        return;

    Mouse::State prev_mouse_state(m_mouse_state);
    m_mouse_state.SetPosition(mouse_position);
    if (m_mouse_state == prev_mouse_state)
        return;

    OnMouseStateChanged(m_mouse_state, m_mouse_state, Mouse::State::Property::Position);
}

void InputControllersPool::MouseInWindowChanged(bool is_mouse_in_window)
{
    if (m_controllers.empty() || m_mouse_in_window == is_mouse_in_window)
        return;

    m_mouse_in_window = is_mouse_in_window;
    OnMouseStateChanged(m_mouse_state, m_mouse_state, Mouse::State::Property::None);
}

void InputControllersPool::OnKeyboardStateChanged(const Keyboard::State& keyboard_state, const Keyboard::State& prev_keyboard_state, Keyboard::State::Property::Mask state_changes_hint)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string("Keyboard: ") + keyboard_state.ToString());
#endif

    for (const InputController::Ptr& sp_controller : m_controllers)
    {
        assert(sp_controller);
        sp_controller->OnKeyboardStateChanged(keyboard_state, prev_keyboard_state, state_changes_hint);
    }
}

void InputControllersPool::OnMouseStateChanged(const Mouse::State& mouse_state, const Mouse::State& prev_mouse_state, Mouse::State::Property::Mask state_changes_hint)
{
#ifdef DEBUG_USER_INPUT
    PrintToDebugOutput(std::string(m_mouse_in_window ? "Mouse in window: " : "Mouse out of window: ") + mouse_state.ToString());
#endif

    for (const InputController::Ptr& sp_controller : m_controllers)
    {
        assert(sp_controller);
        sp_controller->OnMouseStateChanged(mouse_state, prev_mouse_state, state_changes_hint);
    }
}
