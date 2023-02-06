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

FILE: Methane/Platform/Input/Controller.h
Input controller base class for handling keyboard and mouse actions.

******************************************************************************/

#pragma once

#include "IHelpProvider.h"
#include "IController.h"

namespace Methane::Platform::Input
{

class Controller
    : public IController
    , public IHelpProvider
{
public:
    explicit Controller(const std::string& name) : m_name(name) { }

    [[nodiscard]] const std::string& GetControllerName() const { return m_name; }
    [[nodiscard]] bool IsEnabled() const                       { return m_is_enabled; }
    void SetEnabled(bool is_enabled)                           { m_is_enabled = is_enabled; }

    // IController - overrides are optional
    void OnMouseButtonChanged(Mouse::Button /*button*/, Mouse::ButtonState /*button_state*/, const Mouse::StateChange& /*state_change*/) override { /* empty by default */ }
    void OnMousePositionChanged(const Mouse::Position& /*mouse_position*/, const Mouse::StateChange& /*state_change*/) override { /* empty by default */ }
    void OnMouseScrollChanged(const Mouse::Scroll& /*mouse_scroll_delta*/, const Mouse::StateChange& /*state_change*/) override { /* empty by default */ }
    void OnMouseInWindowChanged(bool /*is_mouse_in_window*/, const Mouse::StateChange& /*state_change*/) override { /* empty by default */ }
    void OnKeyboardChanged(Keyboard::Key /*key*/, Keyboard::KeyState /*key_state*/, const Keyboard::StateChange& /*state_change*/) override { /* empty by default */ }
    void OnModifiersChanged(Keyboard::ModifierMask /*modifiers*/, const Keyboard::StateChange& /*state_change*/) override { /* empty by default */ }
    
    // IHelpProvider - overrides are optional
    [[nodiscard]] HelpLines GetHelp() const override { return HelpLines(); }

private:
    std::string m_name;
    bool        m_is_enabled = true;
};

} // namespace Methane::Platform::Input
