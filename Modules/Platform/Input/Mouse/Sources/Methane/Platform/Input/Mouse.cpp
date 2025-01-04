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

FILE: Methane/Platform/Mouse.cpp
Platform abstraction of mouse events.

******************************************************************************/

#include <Methane/Platform/Input/Mouse.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <map>
#include <sstream>

namespace Methane::Platform::Input::Mouse
{

static const std::string g_buttons_separator    = "+";

std::string_view ButtonConverter::ToString() const
{
    META_FUNCTION_TASK();
    switch(m_button)
    {
    case Button::Left:    return "LEFT";
    case Button::Right:   return "RIGHT";
    case Button::Middle:  return "MIDDLE";
    case Button::Button4: return "BUTTON_4";
    case Button::Button5: return "BUTTON_5";
    case Button::Button6: return "BUTTON_6";
    case Button::Button7: return "BUTTON_7";
    case Button::Button8: return "BUTTON_8";
    case Button::VScroll: return "V_SCROLL";
    case Button::HScroll: return "H_SCROLL";
    default:
        META_UNEXPECTED_RETURN_DESCR(m_button, "", "unexpected mouse button");
    }
};

State::State(std::initializer_list<Button> pressed_buttons, const Position& position, const Scroll& scroll, bool in_window)
    : m_position(position)
    , m_scroll(scroll)
    , m_in_window(in_window)
{
    META_FUNCTION_TASK();
    for (Button pressed_button : pressed_buttons)
    {
        SetButton(pressed_button, ButtonState::Pressed);
    }
}

State::PropertyMask State::GetDiff(const State& other) const
{
    META_FUNCTION_TASK();
    State::PropertyMask properties_diff_mask;

    if (m_button_states != other.m_button_states)
        properties_diff_mask.SetBitOn(State::Property::Buttons);
    
    if (m_position != other.m_position)
        properties_diff_mask.SetBitOn(State::Property::Position);

    if (m_scroll != other.m_scroll)
        properties_diff_mask.SetBitOn(State::Property::Scroll);

    if (m_in_window != other.m_in_window)
        properties_diff_mask.SetBitOn(State::Property::InWindow);

    return properties_diff_mask;
}

Buttons State::GetPressedButtons() const
{
    META_FUNCTION_TASK();
    Buttons pressed_buttons;
    for (size_t button_index = 0; button_index < m_button_states.size(); ++button_index)
    {
        if (m_button_states[button_index] != ButtonState::Pressed)
            continue;

        const auto button = static_cast<Button>(button_index);
        pressed_buttons.insert(button);
    }
    return pressed_buttons;
}

std::string State::ToString() const
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "(" << m_position.GetX() << " x " << m_position.GetY() << ")";
    
    bool is_first_button = true;
    for (size_t button_index = 0; button_index < m_button_states.size(); ++button_index)
    {
        if (m_button_states[button_index] != ButtonState::Pressed)
            continue;
        
        if (is_first_button)
        {
            ss << " ";
            is_first_button = false;
        }
        else
            ss << g_buttons_separator;
        
        const auto button = static_cast<Button>(button_index);
        ss << ButtonConverter(button).ToString();
    }

    if (m_scroll.GetX() > 0.1F || m_scroll.GetY() > 0.1F)
    {
        ss << ", scroll=(" << m_scroll.GetX() << " x " << m_scroll.GetY() << ")";
    }

    ss << ", " << (m_in_window ? "in window" : "out of window");
    return ss.str();
}

} // namespace Methane::Platform::Input::Mouse
