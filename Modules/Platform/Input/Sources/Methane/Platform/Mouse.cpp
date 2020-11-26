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

FILE: Methane/Platform/Mouse.cpp
Platform abstraction of mouse events.

******************************************************************************/

#include <Methane/Platform/Mouse.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <map>
#include <sstream>

namespace Methane::Platform::Mouse
{

static const std::string g_buttons_separator    = "+";
static const std::string g_properties_separator = "+";

std::string ButtonConverter::ToString() const
{
    META_FUNCTION_TASK();
    static const std::map<Button, std::string> s_name_by_button =
    {
        { Button::Left,     "Left button"         },
        { Button::Right,    "Right button"        },
        { Button::Middle,   "Middle button"       },
        { Button::Button4,  "Button 4"            },
        { Button::Button5,  "Button 5"            },
        { Button::Button6,  "Button 6"            },
        { Button::Button7,  "Button 7"            },
        { Button::Button8,  "Button 8"            },
        { Button::VScroll,  "Vertical scroll"     },
        { Button::HScroll,  "Horizontal scroll"   },
    };
    
    auto button_and_name_it = s_name_by_button.find(m_button);
    META_CHECK_ARG_DESCR(m_button, button_and_name_it != s_name_by_button.end(), "mouse button name was not found");

    return button_and_name_it->second;
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

bool State::operator==(const State& other) const
{
    META_FUNCTION_TASK();
    return m_button_states == other.m_button_states &&
           m_position      == other.m_position &&
           m_scroll        == other.m_scroll &&
           m_in_window     == other.m_in_window;
}

State::Properties State::GetDiff(const State& other) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    State::Properties properties_diff_mask = State::Properties::None;

    if (m_button_states != other.m_button_states)
        properties_diff_mask |= State::Properties::Buttons;
    
    if (m_position != other.m_position)
        properties_diff_mask |= State::Properties::Position;

    if (m_scroll != other.m_scroll)
        properties_diff_mask |= State::Properties::Scroll;

    if (m_in_window != other.m_in_window)
        properties_diff_mask |= State::Properties::InWindow;

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

        const Button button = static_cast<Button>(button_index);
        pressed_buttons.insert(button);
    }
    return pressed_buttons;
}

std::string State::ToString() const
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "(" << m_position.GetX() << " x " << m_position.GetY() << ") ";
    
    bool is_first_button = true;
    for (size_t button_index = 0; button_index < m_button_states.size(); ++button_index)
    {
        if (m_button_states[button_index] != ButtonState::Pressed)
            continue;
        
        if (!is_first_button)
        {
            ss << g_buttons_separator;
        }
        
        const Button button = static_cast<Button>(button_index);
        ss << ButtonConverter(button).ToString();
        is_first_button = false;
    }

    if (m_scroll.GetX() > 0.1F || m_scroll.GetY() > 0.1F)
    {
        ss << ", scroll=(" << m_scroll.GetX() << " x " << m_scroll.GetY() << ")";
    }

    ss << ", " << (m_in_window ? "in window" : "out of window");
    
    return ss.str();
}

} // namespace Methane::Platform::Mouse
