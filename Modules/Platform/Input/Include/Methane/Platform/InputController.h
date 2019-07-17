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

FILE: Methane/Platform/InputController.h
Abstract input controller interface for handling keyboard and mouse actions.

******************************************************************************/

#pragma once

#include "Keyboard.h"
#include "Mouse.h"

#include <memory>

namespace Methane
{
namespace Platform
{

struct InputController
{
    using Ptr = std::shared_ptr<InputController>;

    virtual void OnKeyboardStateChanged(const Keyboard::State& keyboard_state, const Keyboard::State& prev_keyboard_state, Keyboard::State::Property::Mask state_changes_hint) = 0;
    virtual void OnMouseStateChanged(const Mouse::State& mouse_state, const Mouse::State& prev_mouse_state, Mouse::State::Property::Mask state_changes_hint) = 0;

    virtual ~InputController() = default;
};

} // namespace Platform
} // namespace Methane
