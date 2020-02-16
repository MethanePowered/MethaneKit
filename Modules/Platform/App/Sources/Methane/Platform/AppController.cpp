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

FILE: Methane/Platform/AppController.cpp
Base application controller providing commands like app close and help.

******************************************************************************/

#include <Methane/Platform/AppController.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Version.h>

#include <sstream>
#include <cassert>

namespace Methane::Platform
{

AppController::AppController(AppBase& application, const std::string& application_help, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller(application_help)
    , Keyboard::ActionControllerBase<AppAction>(action_by_keyboard_state, {})
    , m_application(application)
{
    ITT_FUNCTION_TASK();
}

void AppController::OnKeyboardChanged(Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Keyboard::StateChange& state_change)
{
    ITT_FUNCTION_TASK();
    Keyboard::ActionControllerBase<AppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AppController::OnKeyboardStateAction(AppAction action)
{
    ITT_FUNCTION_TASK();
    switch(action)
    {
    case AppAction::ShowControlsHelp:    ShowControlsHelp(); break;
    case AppAction::ShowCommandLineHelp: ShowCommandLineHelp(); break;
    case AppAction::SwitchFullScreen:    m_application.SetFullScreen(!m_application.GetPlatformAppSettings().is_full_screen); break;
    case AppAction::CloseApp:            m_application.Close(); break;
    default: assert(0);
    }
}

std::string AppController::GetKeyboardActionName(AppAction action) const
{
    ITT_FUNCTION_TASK();
    switch (action)
    {
    case AppAction::None:                return "none";
    case AppAction::ShowControlsHelp:    return "show application controls help";
    case AppAction::ShowCommandLineHelp: return "show application command-line help";
    case AppAction::SwitchFullScreen:    return "switch full-screen mode";
    case AppAction::CloseApp:            return "close the application";
    default: assert(0);                  return "";
    }
}

Input::IHelpProvider::HelpLines AppController::GetHelp() const
{
    ITT_FUNCTION_TASK();
    return GetKeyboardHelp();
}

void AppController::ShowControlsHelp()
{
    ITT_FUNCTION_TASK();
    std::stringstream help_stream;
    std::string single_offset = "    ";
    bool is_first_controller = true;
    for (const Ptr<Controller>& sp_controller : m_application.GetInputState().GetControllers())
    {
        assert(!!sp_controller);
        if (!sp_controller) continue;
        
        const HelpLines help_lines = sp_controller->GetHelp();
        if (help_lines.empty()) continue;
        
        if (!is_first_controller)
        {
            help_stream << std::endl;
        }
        is_first_controller = false;
        
        std::string controller_offset;
        if (!sp_controller->GetControllerName().empty())
        {
            help_stream << std::endl << sp_controller->GetControllerName();
            controller_offset = single_offset;
        }
        
        bool first_line = true;
        bool header_present = false;
        for (const KeyDescription& key_description : help_lines)
        {
            if (key_description.first.empty())
            {
                help_stream << std::endl << std::endl << controller_offset << key_description.second << ":";
                header_present = true;
            }
            else
            {
                if (first_line && !header_present)
                {
                    help_stream << std::endl;
                }
                help_stream << std::endl << controller_offset;
                if (header_present)
                {
                    help_stream << single_offset;
                }
                help_stream << key_description.first;
                if (!key_description.second.empty())
                {
                    help_stream << " - " << key_description.second << ";";
                }
            }
            first_line = false;
        }
    }

    if (!is_first_controller)
    {
        help_stream << std::endl;
    }
    help_stream << std::endl << "Powered by Methane Kit v" METHANE_VERSION_STR
                << std::endl << "https://github.com/egorodet/MethaneKit";

    m_application.Alert({
        AppBase::Message::Type::Information,
        "Application Controls Help",
        help_stream.str()
    });
}

void AppController::ShowCommandLineHelp()
{
    ITT_FUNCTION_TASK();
    std::stringstream help_stream;

    const std::string cmd_line_help = m_application.help();
    m_application.Alert({
        AppBase::Message::Type::Information,
        "Application Command-Line Help",
        cmd_line_help
    });
}

} // namespace Methane::Platform
