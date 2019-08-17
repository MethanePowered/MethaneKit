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

FILE: Methane/Graphics/AppContextController.cpp
Graphics context controller for switching parameters in runtime.

******************************************************************************/

#include <Methane/Graphics/AppContextController.h>

#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Platform/Utils.h>

#include <cassert>

using namespace Methane;
using namespace Methane::Graphics;
using namespace Methane::Platform;
using namespace Methane::Platform::Input;

AppContextController::AppContextController(Context& context, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("GRAPHICS SETTINGS")
    , m_context(context)
    , m_action_by_keyboard_state(action_by_keyboard_state)
{
}

void AppContextController::OnKeyboardChanged(Keyboard::Key, Keyboard::KeyState, const Keyboard::StateChange& state_change)
{
    if (state_change.changed_properties != Keyboard::State::Property::KeyStates &&
        state_change.changed_properties != Keyboard::State::Property::Modifiers)
        return;

    const auto action_by_keyboard_state_it = m_action_by_keyboard_state.find(state_change.current);
    if (action_by_keyboard_state_it == m_action_by_keyboard_state.end())
        return;

    switch (action_by_keyboard_state_it->second)
    {
    case Action::SwitchVSync:  m_context.SetVSyncEnabled(!m_context.GetSettings().vsync_enabled); break;
    case Action::SwitchDevice: SwitchDevice(); break;
    default: assert(0); return;
    }
}

void AppContextController::SwitchDevice()
{
    System::Get().UpdateGpuDevices();
    PrintToDebugOutput(System::Get().ToString());
}

IHelpProvider::HelpLines AppContextController::GetHelp() const
{
    HelpLines help_lines;
    if (m_action_by_keyboard_state.empty())
        return help_lines;

    help_lines.reserve(m_action_by_keyboard_state.size());
    for (uint32_t action_index = 0; action_index < static_cast<uint32_t>(Action::Count); ++action_index)
    {
        const Action action = static_cast<Action>(action_index);
        const auto action_by_keyboard_state_it = std::find_if(m_action_by_keyboard_state.begin(), m_action_by_keyboard_state.end(),
            [action](const std::pair<Keyboard::State, Action>& keys_and_action)
            {
                return keys_and_action.second == action;
            });
        if (action_by_keyboard_state_it == m_action_by_keyboard_state.end())
            continue;

        help_lines.push_back({
            action_by_keyboard_state_it->first.ToString(),
            GetActionName(action_by_keyboard_state_it->second)
        });
    }

    return help_lines;
}

std::string AppContextController::GetActionName(Action action)
{
    switch (action)
    {
    case Action::None:         return "none";
    case Action::SwitchVSync:  return "switch vertical synchronization";
    case Action::SwitchDevice: return "switch device used for rendering";
    default: assert(0);        return "";
    }
}
