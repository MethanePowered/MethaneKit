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
 
 FILE: Methane/Platform/KeyboardActionControllerBase.hpp
 Base implementation of the keyboard actions controller.
 
 ******************************************************************************/

#pragma once

#include "Input/HelpProvider.h"
#include "Keyboard.h"

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
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
        META_FUNCTION_TASK();
    }
    
    void OnKeyboardChanged(Key button, KeyState key_state, const StateChange& state_change)
    {
        META_FUNCTION_TASK();
        if (state_change.changed_properties == State::Properties::None)
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
        META_FUNCTION_TASK();
        Input::IHelpProvider::HelpLines help_lines;
        if (m_action_by_keyboard_key.empty() && m_action_by_keyboard_state.empty())
            return help_lines;
        
        help_lines.reserve(m_action_by_keyboard_key.size() + m_action_by_keyboard_state.size());
        for (const ActionEnum action : magic_enum::enum_values<ActionEnum>())
        {
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

    Keyboard::State GetKeyboardStateByAction(ActionEnum action) const noexcept
    {
        META_FUNCTION_TASK();
        const State& key_state = GetKeyboardStateByAction(m_action_by_keyboard_state, action);
        if (key_state)
            return key_state;

        const Key key = GetKeyboardKeyByAction(m_action_by_keyboard_key, action);
        if (key != Key::Unknown)
            return Keyboard::State({ key });

        return Keyboard::State();
    }

    static const State& GetKeyboardStateByAction(const ActionByKeyboardState& action_by_keyboard_state, ActionEnum action) noexcept
    {
        META_FUNCTION_TASK();
        const auto action_by_keyboard_state_it = std::find_if(
            action_by_keyboard_state.begin(), action_by_keyboard_state.end(),
            [action](const auto& keyboard_state_and_action) { return keyboard_state_and_action.second == action; }
        );

        static const Keyboard::State empty_state;
        return action_by_keyboard_state_it == action_by_keyboard_state.end() ? empty_state : action_by_keyboard_state_it->first;
    }

    static Key GetKeyboardKeyByAction(const ActionByKeyboardKey& action_by_key, ActionEnum action) noexcept
    {
        META_FUNCTION_TASK();
        const auto action_by_key_it = std::find_if(
            action_by_key.begin(), action_by_key.end(),
            [action](const auto& key_and_action) { return key_and_action.second == action; }
        );

        return action_by_key_it == action_by_key.end() ? Key::Unknown : action_by_key_it->first;
    }
    
protected:
    // Keyboard::ActionControllerBase interface
    virtual void        OnKeyboardKeyAction(ActionEnum action, KeyState key_state) = 0;
    virtual void        OnKeyboardStateAction(ActionEnum action) = 0;
    virtual std::string GetKeyboardActionName(ActionEnum action) const = 0;

    const ActionByKeyboardKey&   GetActionByKeyboardKey() const noexcept   { return m_action_by_keyboard_key; }
    const ActionByKeyboardState& GetActionByKeyboardState() const noexcept { return m_action_by_keyboard_state; }
    
    ActionEnum GetKeyboardActionByState(State state) const
    {
        META_FUNCTION_TASK();
        const auto action_by_keyboard_state_it = m_action_by_keyboard_state.find(state);
        return (action_by_keyboard_state_it != m_action_by_keyboard_state.end())
              ? action_by_keyboard_state_it->second : ActionEnum::None;
    }
    
    ActionEnum GetKeyboardActionByKey(Key key) const
    {
        META_FUNCTION_TASK();
        const auto action_by_keyboard_key_it = m_action_by_keyboard_key.find(key);
        return (action_by_keyboard_key_it != m_action_by_keyboard_key.end())
        ? action_by_keyboard_key_it->second : ActionEnum::None;
    }

private:
    ActionByKeyboardKey   m_action_by_keyboard_key;
    ActionByKeyboardState m_action_by_keyboard_state;
};

} // namespace Methane::Platform::Keyboard
