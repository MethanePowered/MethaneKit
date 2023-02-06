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

FILE: Methane/Platform/Input/IActionController.h
Abstract input action controller interface for handling keyboard and mouse actions.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Keyboard.h>
#include <Methane/Platform/Input/Mouse.h>

namespace Methane::Platform::Input
{

struct IActionController
{
    virtual void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state) = 0;
    virtual void OnMousePositionChanged(const Mouse::Position& mouse_position) = 0;
    virtual void OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta) = 0;
    virtual void OnMouseInWindowChanged(bool is_mouse_in_window) = 0;
    virtual void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state) = 0;
    virtual void OnModifiersChanged(Keyboard::ModifierMask modifiers) = 0;

    virtual ~IActionController() = default;
};

} // namespace Methane::Platform::Input