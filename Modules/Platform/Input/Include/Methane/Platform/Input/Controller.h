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

FILE: Methane/Platform/Input/Controller.h
Abstract input controller interface for handling keyboard and mouse actions.

******************************************************************************/

#pragma once

#include "HelpProvider.h"

#include <Methane/Platform/Keyboard.h>
#include <Methane/Platform/Mouse.h>

namespace Methane::Platform::Input
{

struct IActionController
{
    virtual void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state) = 0;
    virtual void OnMousePositionChanged(const Mouse::Position& mouse_position) = 0;
    virtual void OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta) = 0;
    virtual void OnMouseInWindowChanged(bool is_mouse_in_window) = 0;
    virtual void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state) = 0;
    virtual void OnModifiersChanged(Keyboard::Modifier::Mask modifiers) = 0;

    virtual ~IActionController() = default;
};

struct IController
{
    virtual void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state, const Mouse::StateChange& state_change) = 0;
    virtual void OnMousePositionChanged(const Mouse::Position& mouse_position, const Mouse::StateChange& state_change) = 0;
    virtual void OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta, const Mouse::StateChange& state_change) = 0;
    virtual void OnMouseInWindowChanged(bool is_mouse_in_window, const Mouse::StateChange& state_change) = 0;
    virtual void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state, const Keyboard::StateChange& state_change) = 0;
    virtual void OnModifiersChanged(Keyboard::Modifier::Mask modifiers, const Keyboard::StateChange& state_change) = 0;

    virtual ~IController() = default;
};

class Controller
    : public IController
    , public IHelpProvider
{
public:
    Controller(const std::string& name) : m_name(name) { }

    const std::string& GetControllerName() const { return m_name; }
    bool IsEnabled() const { return m_is_enabled; }
    void SetEnabled(bool is_enabled) { m_is_enabled = is_enabled; }

    // IController - overrides are optional
    void OnMouseButtonChanged(Mouse::Button /*button*/, Mouse::ButtonState /*button_state*/, const Mouse::StateChange& /*state_change*/) override { }
    void OnMousePositionChanged(const Mouse::Position& /*mouse_position*/, const Mouse::StateChange& /*state_change*/) override { }
    void OnMouseScrollChanged(const Mouse::Scroll& /*mouse_scroll_delta*/, const Mouse::StateChange& /*state_change*/) override { }
    void OnMouseInWindowChanged(bool /*is_mouse_in_window*/, const Mouse::StateChange& /*state_change*/) override { }
    void OnKeyboardChanged(Keyboard::Key /*key*/, Keyboard::KeyState /*key_state*/, const Keyboard::StateChange& /*state_change*/) override { }
    void OnModifiersChanged(Keyboard::Modifier::Mask /*modifiers*/, const Keyboard::StateChange& /*state_change*/) override { }
    
    // IHelpProvider - overrides are optional
    HelpLines GetHelp() const override { return HelpLines(); }

private:
    std::string m_name;
    bool        m_is_enabled = true;
};

} // namespace Methane::Platform::Input
