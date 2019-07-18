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

FILE: Methane/Platform/Input/State.h
Aggregated application input state with controllers.

******************************************************************************/

#pragma once

#include "ControllersPool.h"

namespace Methane
{
namespace Platform
{
namespace Input
{

class State
{
public:
    State() = default;
    State(const ControllersPool& controllers) : m_controllers(controllers) {}

    ControllersPool& GetControllers() { return m_controllers; }
    void SetControllers(const Controllers& controllers) { m_controllers = controllers; }

    void KeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state);
    void MouseButtonsChanged(Mouse::Button button, Mouse::ButtonState button_state);
    void MousePositionChanged(Mouse::Position mouse_position);
    void MouseInWindowChanged(bool is_mouse_in_window);
    
    const Keyboard::State&  GetKeyboardState() const { return m_keyboard_state; }
    const Mouse::State&     GetMouseState() const    { return m_mouse_state; }

protected:
    ControllersPool  m_controllers;
    Keyboard::State  m_keyboard_state;
    Mouse::State     m_mouse_state;
    bool             m_mouse_in_window = false;
};

} // namespace Input
} // namespace Platform
} // namespace Methane
