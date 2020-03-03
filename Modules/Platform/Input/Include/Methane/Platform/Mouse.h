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

FILE: Methane/Platform/Mouse.h
Platform abstraction of mouse events.

******************************************************************************/

#pragma once

#include <Methane/Data/Types.h>

#include <cmath>
#include <array>
#include <set>
#include <string>
#include <sstream>

namespace Methane::Platform::Mouse
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
    VScroll,
    HScroll,

    Count,
    Unknown
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
using Scroll = Data::Point2f;


using MouseButtonAndDelta = std::pair<Mouse::Button, float>;
inline MouseButtonAndDelta GetScrollButtonAndDelta(const Scroll& scroll_delta)
{
    const float min_scroll_delta = 0.00001f;
    return std::fabs(scroll_delta.GetY()) > min_scroll_delta ? MouseButtonAndDelta(Button::VScroll, scroll_delta.GetY())
                                                             : (std::fabs(scroll_delta.GetX()) > min_scroll_delta ? MouseButtonAndDelta(Button::HScroll, scroll_delta.GetX())
                                                             : MouseButtonAndDelta(Button::Unknown, 0.f ) );
}

class State
{
public:
    struct Property
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            None        = 0u,
            Buttons     = 1u << 0u,
            Position    = 1u << 1u,
            Scroll      = 1u << 2u,
            InWindow    = 1u << 3u,
            All         = ~0u,
        };

        using Values = std::array<Value, 4>;
        static constexpr const Values values = { Buttons, Position, Scroll, InWindow };

        static std::string ToString(Value property_value);
        static std::string ToString(Mask properties_mask);

        Property() = delete;
        ~Property() = delete;
    };

    State() = default;
    State(std::initializer_list<Button> pressed_buttons, const Position& position = Position(), const Scroll& scroll = Scroll(), bool in_window = false);
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

    const Scroll&       GetScroll() const                       { return m_scroll; }
    void                AddScrollDelta(const Scroll& delta)     { m_scroll += delta; }
    void                ResetScroll();

    bool                IsInWindow() const                      { return m_in_window; }
    void                SetInWindow(bool in_window)             { m_in_window = in_window; }

    Buttons             GetPressedButtons() const;
    const ButtonStates& GetButtonStates() const                 { return m_button_states; }
    Property::Mask      GetDiff(const State& other) const;
    std::string         ToString() const;

private:
    ButtonStates m_button_states    = {};
    Position     m_position         = {};
    Scroll       m_scroll           = {};
    bool         m_in_window        = false;
};

inline std::ostream& operator<<( std::ostream& os, State const& keyboard_state)
{
    os << keyboard_state.ToString();
    return os;
}

struct StateChange
{
    StateChange(const State& in_current, const State& in_previous, State::Property::Mask in_changed_properties)
        : current(in_current)
        , previous(in_previous)
        , changed_properties(in_changed_properties)
    { }

    const State& current;
    const State& previous;
    const State::Property::Mask changed_properties;
};

} // namespace Methane::Platform::Mouse
