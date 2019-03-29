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

FILE: Methane/Platform/Mouse.h
Platform abstraction of mouse events.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>

#include <array>
#include <set>

namespace Methane
{
namespace Platform
{
namespace Mouse
{

enum class Button : uint32_t
{
    Left        = 0,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,

    Count
};

using Buttons = std::set<Button>;

enum class ButtonState : uint32_t
{
    Pressed     = 0,
    Released
};

using ButtonStates = std::array<ButtonState, static_cast<size_t>(Button::Count)>;

using Position = Data::Point2i;

class State
{
public:
    ButtonState GetButtonState(Button button) const                 { return m_button_states[static_cast<size_t>(button)];  }
    void        SetButtonState(Button button, ButtonState state)    { m_button_states[static_cast<size_t>(button)] = state; }

    const Position& GetPosition() const                             { return m_position; }
    void            SetPosition(const Position& position)           { m_position = position; }

private:
    ButtonStates m_button_states;
    Position     m_position;
};

} // namespace Mouse
} // namespace Platform
} // namespace Methane
