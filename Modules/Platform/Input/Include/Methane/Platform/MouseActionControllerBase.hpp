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
 
 FILE: Methane/Platform/MouseActionControllerBase.hpp
 Base implementation of the mouse actions controller.
 
 ******************************************************************************/

#pragma once

#include "Input/HelpProvider.h"
#include "Mouse.h"

#include <map>

namespace Methane::Platform::Mouse
{

template<typename ActionEnum>
class ActionControllerBase
{
public:
    using ActionByMouseButton = std::map<Button, ActionEnum>;
    
    ActionControllerBase(const ActionByMouseButton& action_by_mouse_button)
        : m_action_by_mouse_button(action_by_mouse_button)
    {
        ITT_FUNCTION_TASK();
    }
    
    Input::IHelpProvider::HelpLines GetMouseHelp() const
    {
        ITT_FUNCTION_TASK();
        Input::IHelpProvider::HelpLines help_lines;
        if (m_action_by_mouse_button.empty())
            return help_lines;
        
        help_lines.reserve(m_action_by_mouse_button.size());
        for (uint32_t action_index = 0; action_index < static_cast<uint32_t>(ActionEnum::Count); ++action_index)
        {
            const ActionEnum action = static_cast<ActionEnum>(action_index);
            const auto action_by_mouse_button_it = std::find_if(m_action_by_mouse_button.begin(), m_action_by_mouse_button.end(),
                                                                [action](const std::pair<Button, ActionEnum>& button_and_action)
                                                                {
                                                                    return button_and_action.second == action;
                                                                });
            if (action_by_mouse_button_it == m_action_by_mouse_button.end())
                continue;
            
            help_lines.push_back({
                Mouse::ButtonConverter(action_by_mouse_button_it->first).ToString(),
                GetMouseActionName(action_by_mouse_button_it->second)
            });
        }
        
        return help_lines;
    }
    
protected:
    // Mouse::ActionControllerBase interface
    virtual std::string GetMouseActionName(ActionEnum action) const = 0;
    
    ActionEnum GetMouseActionByButton(Button mouse_button) const
    {
        ITT_FUNCTION_TASK();
        const auto action_by_mouse_button_it = m_action_by_mouse_button.find(mouse_button);
        return (action_by_mouse_button_it != m_action_by_mouse_button.end())
              ? action_by_mouse_button_it->second : ActionEnum::None;
    }
    
    ActionByMouseButton m_action_by_mouse_button;
};

} // namespace Methane::Platform::Mouse
