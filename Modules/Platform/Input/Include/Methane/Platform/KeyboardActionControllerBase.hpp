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
 
 FILE: Methane/Platform/KeyboardActionControllerBase.hpp
 Base implementation of the keyboard actions controller.
 
 ******************************************************************************/

#pragma once

#include "Input/HelpProvider.h"
#include "Keyboard.h"

#include <Methane/Instrumentation.h>

#include <map>

namespace Methane::Platform::Keyboard
{

template<typename ActionEnum>
class ActionControllerBase
{
public:
    using ActionByKeyboardState = std::map<State, ActionEnum>;
    using ActionByKeyboardKey   = std::map<Key,   ActionEnum>;
    
    ActionControllerBase(const ActionByKeyboardState& action_by_keyboard_state,
                         const ActionByKeyboardKey&   action_by_keyboard_key)
        : m_action_by_keyboard_key(action_by_keyboard_key)
        , m_action_by_keyboard_state(action_by_keyboard_state)
    {
        ITT_FUNCTION_TASK();
    }
    
    void OnKeyboardChanged(Key button, KeyState key_state, const StateChange& state_change)
    {
        ITT_FUNCTION_TASK();
        if (state_change.changed_properties == State::Property::None)
            return;
        
        const auto action_by_keyboard_state_it = m_action_by_keyboard_state.find(state_change.current);
        if (action_by_keyboard_state_it != m_action_by_keyboard_state.end())
        {
            OnKeyboardStateAction(action_by_keyboard_state_it->second);
        }
        
        const auto action_by_keyboard_key_it = m_action_by_keyboard_key.find(button);
        if (action_by_keyboard_key_it != m_action_by_keyboard_key.end())
        {
            OnKeyboardKeyAction(action_by_keyboard_key_it->second, key_state);
        }
    }
    
    Input::IHelpProvider::HelpLines GetKeyboardHelp() const
    {
        ITT_FUNCTION_TASK();
        Input::IHelpProvider::HelpLines help_lines;
        if (m_action_by_keyboard_key.empty() && m_action_by_keyboard_state.empty())
            return help_lines;
        
        help_lines.reserve(m_action_by_keyboard_key.size() + m_action_by_keyboard_state.size());
        for (uint32_t action_index = 0; action_index < static_cast<uint32_t>(ActionEnum::Count); ++action_index)
        {
            const ActionEnum action = static_cast<ActionEnum>(action_index);
            
            const auto action_by_keyboard_state_it = std::find_if(m_action_by_keyboard_state.begin(), m_action_by_keyboard_state.end(),
                                                                  [action](const std::pair<Keyboard::State, ActionEnum>& keys_and_action)
                                                                  { return keys_and_action.second == action; });
            if (action_by_keyboard_state_it != m_action_by_keyboard_state.end())
            {
                help_lines.push_back({
                    action_by_keyboard_state_it->first.ToString(),
                    GetKeyboardActionName(action_by_keyboard_state_it->second)
                });
            }
            
            const auto action_by_keyboard_key_it = std::find_if(m_action_by_keyboard_key.begin(), m_action_by_keyboard_key.end(),
                                                                [action](const std::pair<Keyboard::Key, ActionEnum>& key_and_action)
                                                                { return key_and_action.second == action; });
            if (action_by_keyboard_key_it != m_action_by_keyboard_key.end())
            {
                help_lines.push_back({
                    Keyboard::KeyConverter(action_by_keyboard_key_it->first).ToString(),
                    GetKeyboardActionName(action_by_keyboard_key_it->second)
                });
            }
        }
        
        return help_lines;
    }
    
protected:
    // Keyboard::ActionControllerBase interface
    virtual void        OnKeyboardKeyAction(ActionEnum action, KeyState key_state) = 0;
    virtual void        OnKeyboardStateAction(ActionEnum action) = 0;
    virtual std::string GetKeyboardActionName(ActionEnum action) const = 0;
    
    ActionEnum GetKeyboardActionByState(State state) const
    {
        ITT_FUNCTION_TASK();
        const auto action_by_keyboard_state_it = m_action_by_keyboard_state.find(state);
        return (action_by_keyboard_state_it != m_action_by_keyboard_state.end())
              ? action_by_keyboard_state_it->second : ActionEnum::None;
    }
    
    ActionEnum GetKeyboardActionByKey(Key key) const
    {
        ITT_FUNCTION_TASK();
        const auto action_by_keyboard_key_it = m_action_by_keyboard_key.find(key);
        return (action_by_keyboard_key_it != m_action_by_keyboard_key.end())
        ? action_by_keyboard_key_it->second : ActionEnum::None;
    }
    
    ActionByKeyboardKey   m_action_by_keyboard_key;
    ActionByKeyboardState m_action_by_keyboard_state;
};

} // namespace Methane::Platform::Keyboard
