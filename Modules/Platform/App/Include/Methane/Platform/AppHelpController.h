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

FILE: Methane/Platform/AppHelpController.h
Help displaying controller.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/KeyboardActionControllerBase.hpp>

namespace Methane
{
namespace Platform
{
    
enum class AppHelpAction : uint32_t
{
    None = 0,
    
    ShowHelp,
    CloseApp,
    
    Count
};

class AppHelpController final
    : public Input::Controller
    , public Platform::Keyboard::ActionControllerBase<AppHelpAction>
{
public:
    inline static const ActionByKeyboardState default_action_by_keyboard_state = {
        { { Platform::Keyboard::Key::F1 },                                    AppHelpAction::ShowHelp  },
        { { Platform::Keyboard::Key::LeftSuper, Platform::Keyboard::Key::Q }, AppHelpAction::CloseApp  },
    };
    
    AppHelpController(AppBase& application, const std::string& application_help, bool show_command_line_help = false,
                      const ActionByKeyboardState& action_by_keyboard_state = default_action_by_keyboard_state);
    
    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key, Platform::Keyboard::KeyState, const Platform::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;
    
    void ShowHelp();

private:
    // Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(AppHelpAction, Platform::Keyboard::KeyState) override { }
    void        OnKeyboardStateAction(AppHelpAction action) override;
    std::string GetKeyboardActionName(AppHelpAction action) const override;

    AppBase&            m_application;
    const bool          m_show_command_line_help;
};

} // namespace Platform
} // namespace Methane
