/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/AppController.cpp
Base user interface application controller

******************************************************************************/

#include <Methane/UserInterface/AppController.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::UserInterface
{

AppController::AppController(IApp& application, const std::string& application_help,
                             const Platform::AppController::ActionByKeyboardState& platform_action_by_keyboard_state,
                             const Graphics::AppController::ActionByKeyboardState& graphics_action_by_keyboard_state,
                             const UserInterface::AppController::ActionByKeyboardState& ui_action_by_keyboard_state)
    : Graphics::AppController(application, application_help, platform_action_by_keyboard_state, graphics_action_by_keyboard_state)
    , Platform::Keyboard::ActionControllerBase<AppAction>(ui_action_by_keyboard_state, {})
    , m_application(application)
{
    META_FUNCTION_TASK();
}

void AppController::OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Platform::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Platform::Keyboard::ActionControllerBase<AppAction>::OnKeyboardChanged(key, key_state, state_change);
    Graphics::AppController::OnKeyboardChanged(key, key_state, state_change);
}

void AppController::OnKeyboardStateAction(AppAction action)
{
    META_FUNCTION_TASK();
    switch(action)
    {
    case AppAction::SwitchHeadsUpDisplayMode:
        m_application.SetHeadsUpDisplayMode(static_cast<IApp::HeadsUpDisplayMode>(
            (static_cast<uint32_t>(m_application.GetUserInterfaceAppSettings().heads_up_display_mode) + 1) % IApp::HeadsUpDisplayMode::Count));
        break;

    default:
        META_UNEXPECTED_ENUM_ARG(action);
    }
}

std::string AppController::GetKeyboardActionName(AppAction action) const
{
    META_FUNCTION_TASK();
    switch (action)
    {
    case AppAction::None:                       return "none";
    case AppAction::SwitchHeadsUpDisplayMode:   return "switch heads-up-display mode";
    default:                                    META_UNEXPECTED_ENUM_ARG_RETURN(action, "");
    }
}

Platform::Input::IHelpProvider::HelpLines AppController::GetHelp() const
{
    META_FUNCTION_TASK();
    HelpLines help_lines = Graphics::AppController::GetHelp();
    const HelpLines gui_help_lines = Platform::Keyboard::ActionControllerBase<AppAction>::GetKeyboardHelp();
    help_lines.insert(help_lines.end(), std::make_move_iterator(gui_help_lines.begin()), std::make_move_iterator(gui_help_lines.end()));
    return help_lines;
}

} // namespace Methane::Platform
