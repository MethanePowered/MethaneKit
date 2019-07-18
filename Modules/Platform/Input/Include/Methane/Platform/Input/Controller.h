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

FILE: Methane/Platform/Input/Controller.h
Abstract input controller interface for handling keyboard and mouse actions.

******************************************************************************/

#pragma once

#include <Methane/Platform/Keyboard.h>
#include <Methane/Platform/Mouse.h>

#include <memory>
#include <vector>

namespace Methane
{
namespace Platform
{
namespace Input
{

struct IController
{
    virtual void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state, const Mouse::StateChange& state_change) = 0;
    virtual void OnMousePositionChanged(const Mouse::Position& mouse_position, const Mouse::StateChange& state_change) = 0;
    virtual void OnMouseInWindowChanged(bool is_mouse_in_window, const Mouse::State& mouse_state) = 0;
    virtual void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state, const Keyboard::StateChange& state_change) = 0;

    virtual ~IController() = default;
};

class Controller : public IController
{
public:
    using Ptr = std::shared_ptr<Controller>;

    bool IsEnabled() const { return m_is_enabled; }
    void SetEnabled(bool is_enabled) { m_is_enabled = is_enabled; }

    // IController - overrides are optional
    void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state, const Mouse::StateChange& state_change) override { }
    void OnMousePositionChanged(const Mouse::Position& mouse_position, const Mouse::StateChange& state_change) override { }
    void OnMouseInWindowChanged(bool is_mouse_in_window, const Mouse::State& mouse_state) override { }
    void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state, const Keyboard::StateChange& state_change) override { }

private:
    bool m_is_enabled = true;
};

using Controllers = std::vector<Controller::Ptr>;

} // namespace Input
} // namespace Platform
} // namespace Methane
