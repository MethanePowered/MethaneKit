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

FILE: Methane/Platform/AppHelpController.cpp
Help displaying controller.

******************************************************************************/

#include <Methane/Platform/AppHelpController.h>

#include <sstream>
#include <cassert>

using namespace Methane;
using namespace Methane::Platform;
using namespace Methane::Platform::Input;

AppHelpController::AppHelpController(AppBase& application, const std::string& application_help, bool show_command_line_help,
                                     const ActionByKeyboardState& action_by_keyboard_state)
    : Input::Controller(application_help)
    , Keyboard::ActionControllerBase<AppHelpAction>(action_by_keyboard_state, {})
    , m_application(application)
    , m_show_command_line_help(show_command_line_help)
{
}

void AppHelpController::OnKeyboardChanged(Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Keyboard::StateChange& state_change)
{
    Keyboard::ActionControllerBase<AppHelpAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AppHelpController::OnKeyboardStateAction(AppHelpAction action)
{
    if (action != AppHelpAction::ShowHelp)
        return;
    
    std::stringstream help_stream;
    std::string single_offset = "    ";
    bool is_first_controller = true;
    for (const Controller::Ptr& sp_controller : m_application.GetInputState().GetControllers())
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
            help_stream << sp_controller->GetControllerName() << std::endl;
            controller_offset = single_offset;
        }
        
        bool first_line = true;
        bool header_present = false;
        for (const KeyDescription& key_description : help_lines)
        {
            if (key_description.first.empty())
            {
                help_stream << std::endl << controller_offset << key_description.second << ":" << std::endl;
                header_present = true;
            }
            else
            {
                if (first_line && !header_present)
                {
                    help_stream << std::endl;
                }
                help_stream << controller_offset;
                if (header_present)
                {
                    help_stream << single_offset;
                }
                help_stream << key_description.first;
                if (!key_description.second.empty())
                {
                    help_stream << " - " << key_description.second << ";";
                }
                help_stream << std::endl;
            }
            first_line = false;
        }
    }
    
    if (m_show_command_line_help)
    {
        const std::string cmd_line_help = m_application.GetCmdOptions().help();
        if (!cmd_line_help.empty())
        {
            if (!is_first_controller)
            {
                help_stream << std::endl;
            }
            help_stream << "COMMAND LINE OPTIONS" << std::endl;
            help_stream << std::endl << cmd_line_help;
        }
    }
    
    m_application.Alert({
        AppBase::Message::Type::Information,
        "Application Help",
        help_stream.str()
    });
}

std::string AppHelpController::GetKeyboardActionName(AppHelpAction action) const
{
    switch (action)
    {
        case AppHelpAction::None:      return "none";
        case AppHelpAction::ShowHelp:  return "show application help";
        default: assert(0);            return "";
    }
}

IHelpProvider::HelpLines AppHelpController::GetHelp() const
{
    return GetKeyboardHelp();
}
