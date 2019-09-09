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

namespace Methane::Platform
{
namespace Input
{

class State : public IActionController
{
public:
    State() = default;
    State(const ControllersPool& controllers) : m_controllers(controllers) {}

    const ControllersPool&  GetControllers() const                              { return m_controllers; }
    void                    AddControllers(const Controllers& controllers)      { m_controllers.insert(m_controllers.end(), controllers.begin(), controllers.end()); }

    const Keyboard::State& GetKeyboardState() const                 { return m_keyboard_state; }
    const Mouse::State&    GetMouseState() const                    { return m_mouse_state; }

    // IActionController
    void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state) override;
    void OnMousePositionChanged(const Mouse::Position& mouse_position) override;
    void OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta) override;
    void OnMouseInWindowChanged(bool is_mouse_in_window) override;
    void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state) override;
    void OnModifiersChanged(Keyboard::Modifier::Mask modifiers) override;

    void ReleaseAllKeys();

protected:
    ControllersPool     m_controllers;
    Mouse::State        m_mouse_state;
    Keyboard::StateExt  m_keyboard_state;
};

} // namespace Input
} // namespace Methane::Platform
