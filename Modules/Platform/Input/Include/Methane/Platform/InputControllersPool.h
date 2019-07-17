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

FILE: Methane/Platform/InputControllersPool.h
A pool of input controllers for user actions handling in a separated model components.

******************************************************************************/

#pragma once

#include "InputController.h"

#include <vector>

namespace Methane
{
namespace Platform
{

class InputControllersPool
{
public:
    using Controllers = std::vector<InputController::Ptr>;

    const Controllers& GetControllers() const { return m_controllers; }
    void AddController(InputController::Ptr&& sp_controller);

    void KeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state);
    void MouseButtonsChanged(Mouse::Button button, Mouse::ButtonState button_state);
    void MousePositionChanged(Mouse::Position mouse_position);
    void MouseInWindowChanged(bool is_mouse_in_window);

protected:
    void OnKeyboardStateChanged(const Keyboard::State& keyboard_state, const Keyboard::State& prev_keyboard_state, Keyboard::State::Property::Mask state_changes_hint);
    void OnMouseStateChanged(const Mouse::State& mouse_state, const Mouse::State& prev_mouse_state, Mouse::State::Property::Mask state_changes_hint);

    Controllers      m_controllers;
    Keyboard::State  m_keyboard_state;
    Mouse::State     m_mouse_state;
    bool             m_mouse_in_window = false;
};

} // namespace Platform
} // namespace Methane
