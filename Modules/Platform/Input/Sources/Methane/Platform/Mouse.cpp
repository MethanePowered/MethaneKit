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

FILE: Methane/Platform/Mouse.cpp
Platform abstraction of mouse events.

******************************************************************************/

#include <Methane/Platform/Mouse.h>

#include <map>
#include <sstream>
#include <cassert>

using namespace Methane::Platform::Mouse;

static const std::string s_buttons_separator = "+";

std::string ButtonConverter::ToString() const
{
    static const std::map<Button, std::string> s_name_by_button =
    {
        { Button::Left,       "Left"    },
        { Button::Right,      "Right"   },
        { Button::Middle,     "Middle"  },
        { Button::Button4,    "Button4" },
        { Button::Button5,    "Button5" },
        { Button::Button6,    "Button6" },
        { Button::Button7,    "Button7" },
        { Button::Button8,    "Button8" },
    };
    
    auto button_and_name_it = s_name_by_button.find(m_button);
    if (button_and_name_it == s_name_by_button.end())
    {
        assert(0);
        return "";
    }
    return button_and_name_it->second;
};

State::State(std::initializer_list<Button> pressed_buttons, const Position& position)
    : m_position(position)
{
    for (Button pressed_button : pressed_buttons)
    {
        SetButton(pressed_button, ButtonState::Pressed);
    }
}

State::State(const State& other)
    : m_button_states(other.m_button_states)
    , m_position(other.m_position)
{
}

Buttons State::GetPressedButtons() const
{
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
    std::stringstream ss;
    ss << "Mouse (" << m_position.x() << " x " << m_position.y() << ") ";
    
    bool is_first_button = true;
    for (size_t button_index = 0; button_index < m_button_states.size(); ++button_index)
    {
        if (m_button_states[button_index] != ButtonState::Pressed)
            continue;
        
        if (!is_first_button)
        {
            ss << s_buttons_separator;
        }
        
        const Button button = static_cast<Button>(button_index);
        ss << ButtonConverter(button).ToString();
        is_first_button = false;
    }
    
    return ss.str();
}
