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

FILE: Methane/Platform/Input/State.h
Aggregated application input state with controllers.

******************************************************************************/

#pragma once

#include "ControllersPool.h"

#include <Methane/Instrumentation.h>

namespace Methane::Platform::Input
{

class State : public IActionController
{
public:
    State() = default;
    State(const ControllersPool& controllers) : m_controllers(controllers) {}

    const ControllersPool&  GetControllers() const noexcept                     { return m_controllers; }
    void                    AddControllers(const Ptrs<Controller>& controllers) { m_controllers.insert(m_controllers.end(), controllers.begin(), controllers.end()); }

    const Keyboard::State& GetKeyboardState() const noexcept { return m_keyboard_state; }
    const Mouse::State&    GetMouseState() const noexcept    { return m_mouse_state; }

    // IActionController
    void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state) override;
    void OnMousePositionChanged(const Mouse::Position& mouse_position) override;
    void OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta) override;
    void OnMouseInWindowChanged(bool is_mouse_in_window) override;
    void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state) override;
    void OnModifiersChanged(Keyboard::Modifier::Mask modifiers) override;

    void ReleaseAllKeys();

    template<typename ControllerT, typename = std::enable_if_t<std::is_base_of_v<Controller, ControllerT>>>
    Refs<ControllerT> GetControllersOfType() const
    {
        ITT_FUNCTION_TASK();
        Refs<ControllerT> controllers;
        const std::type_info& controller_type  = typeid(ControllerT);
        for(const Ptr<Controller>& sp_controller : m_controllers)
        {
            if (!sp_controller)
                continue;
            Controller& controller = *sp_controller;
            if (typeid(controller) == controller_type)
                controllers.emplace_back(static_cast<ControllerT&>(*sp_controller));
        }
        return controllers;
    }

protected:
    ControllersPool     m_controllers;
    Mouse::State        m_mouse_state;
    Keyboard::StateExt  m_keyboard_state;
};

} // namespace Methane::Platform::Input
