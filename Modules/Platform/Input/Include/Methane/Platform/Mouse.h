/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Platform/Mouse.h
Platform abstraction of mouse events.

******************************************************************************/

#pragma once

#include <Methane/Data/Point.hpp>
#include <Methane/Data/EnumMask.hpp>

#include <cmath>
#include <array>
#include <set>
#include <string>
#include <string_view>
#include <sstream>

namespace Methane::Platform::Mouse
{

enum class Button : uint32_t
{
    Left = 0U,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,
    VScroll,
    HScroll,

    Unknown
};

using Buttons = std::set<Button>;

class ButtonConverter
{
public:
    explicit ButtonConverter(Button button) : m_button(button) { }

    [[nodiscard]] std::string_view ToString() const;
    
private:
    Button m_button;
};

enum class ButtonState : uint8_t
{
    Released = 0,
    Pressed,
};

using ButtonStates = std::array<ButtonState, magic_enum::enum_count<Button>() - 1>;

using Position = Data::Point2I;
using Scroll = Data::Point2F;

using MouseButtonAndDelta = std::pair<Mouse::Button, float>;
[[nodiscard]] inline MouseButtonAndDelta GetScrollButtonAndDelta(const Scroll& scroll_delta)
{
    constexpr float min_scroll_delta = 0.00001F;
    if (std::fabs(scroll_delta.GetY()) > min_scroll_delta)
        return MouseButtonAndDelta(Button::VScroll, scroll_delta.GetY());

    return std::fabs(scroll_delta.GetX()) > min_scroll_delta
         ? MouseButtonAndDelta(Button::HScroll, scroll_delta.GetX())
         : MouseButtonAndDelta(Button::Unknown, 0.F);
}

class State
{
public:
    enum class Property : uint32_t
    {
        Buttons,
        Position,
        Scroll,
        InWindow
    };

    using PropertyMask = Data::EnumMask<Property>;

    State() = default;
    State(std::initializer_list<Button> pressed_buttons, const Position& position = Position(), const Scroll& scroll = Scroll(), bool in_window = false);

    [[nodiscard]] bool operator==(const State& other) const;
    [[nodiscard]] bool operator!=(const State& other) const          { return !operator==(other); }
    [[nodiscard]] const ButtonState& operator[](Button button) const { return m_button_states[static_cast<size_t>(button)]; }
    [[nodiscard]] explicit operator std::string() const              { return ToString(); }

    void SetButton(Button button, ButtonState state) { m_button_states[static_cast<size_t>(button)] = state; }
    void PressButton(Button button)                  { SetButton(button, ButtonState::Pressed); }
    void ReleaseButton(Button button)                { SetButton(button, ButtonState::Released); }
    void SetPosition(const Position& position)       { m_position = position; }
    void SetInWindow(bool in_window)                 { m_in_window = in_window; }
    void AddScrollDelta(const Scroll& delta)         { m_scroll += delta; }
    void ResetScroll();

    [[nodiscard]] const Position&     GetPosition() const                     { return m_position; }
    [[nodiscard]] const Scroll&       GetScroll() const                       { return m_scroll; }
    [[nodiscard]] bool                IsInWindow() const                      { return m_in_window; }
    [[nodiscard]] const ButtonStates& GetButtonStates() const                 { return m_button_states; }
    [[nodiscard]] Buttons             GetPressedButtons() const;
    [[nodiscard]] PropertyMask          GetDiff(const State& other) const;
    [[nodiscard]] std::string         ToString() const;

private:
    ButtonStates m_button_states { };
    Position     m_position      { };
    Scroll       m_scroll        { };
    bool         m_in_window     = false;
};

inline std::ostream& operator<<( std::ostream& os, State const& keyboard_state)
{
    os << keyboard_state.ToString();
    return os;
}

struct StateChange
{
    StateChange(const State& in_current, const State& in_previous, State::PropertyMask in_changed_properties)
        : current(in_current)
        , previous(in_previous)
        , changed_properties(in_changed_properties)
    { }

    const State& current;
    const State& previous;
    const State::PropertyMask changed_properties;
};

} // namespace Methane::Platform::Mouse
