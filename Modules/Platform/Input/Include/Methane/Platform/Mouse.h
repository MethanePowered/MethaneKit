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
#include <string>

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

class ButtonConverter
{
public:
    ButtonConverter(Button button) : m_button(button) { }
    
    std::string ToString() const;
    
private:
    Button m_button;
};

enum class ButtonState : uint8_t
{
    Released = 0,
    Pressed,
};

using ButtonStates = std::array<ButtonState, static_cast<size_t>(Button::Count)>;

using Position = Data::Point2i;

class State
{
public:
    State() = default;
    State(std::initializer_list<Button> pressed_buttons, const Position& position = Position());
    State(const State& other);

    State& operator=(const State& other);
    bool operator==(const State& other) const;
    bool operator!=(const State& other) const                   { return !operator==(other); }
    const ButtonState& operator[](Button button) const          { return m_button_states[static_cast<size_t>(button)]; }
    operator std::string() const                                { return ToString(); }

    void  SetButton(Button button, ButtonState state)           { m_button_states[static_cast<size_t>(button)] = state; }
    void  PressButton(Button button)                            { SetButton(button, ButtonState::Pressed); }
    void  ReleaseButton(Button button)                          { SetButton(button, ButtonState::Released); }
    
    const Position&     GetPosition() const                     { return m_position; }
    void                SetPosition(const Position& position)   { m_position = position; }

    Buttons             GetPressedButtons() const;
    const ButtonStates& GetButtonStates() const                 { return m_button_states; }
    std::string         ToString() const;

private:
    ButtonStates m_button_states{};
    Position     m_position{};
};

inline std::ostream& operator<<( std::ostream& os, State const& keyboard_state)
{
    os << keyboard_state.ToString();
    return os;
}

} // namespace Mouse
} // namespace Platform
} // namespace Methane
