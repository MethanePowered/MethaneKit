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

FILE: Methane/Platform/AppController.h
Base application controller providing commands like app close and help.

******************************************************************************/

#pragma once

#include <Methane/Platform/AppBase.h>
#include <Methane/Platform/Input/KeyboardActionControllerBase.hpp>

namespace Methane::Platform
{

enum class AppAction
{
    None,
    ShowControlsHelp,
    ShowCommandLineHelp,
    ShowParameters,
    SwitchFullScreen,
    CloseApp
};

class AppController
    : public Input::Controller
    , public Input::Keyboard::ActionControllerBase<AppAction>
{
public:
    inline static const ActionByKeyboardState default_action_by_keyboard_state {
        { { Input::Keyboard::Key::F1 },                                      AppAction::ShowControlsHelp    },
        { { Input::Keyboard::Key::F2 },                                      AppAction::ShowCommandLineHelp },
        { { Input::Keyboard::Key::F3 },                                      AppAction::ShowParameters      },
        { { Input::Keyboard::Key::LeftControl,    Input::Keyboard::Key::F }, AppAction::SwitchFullScreen    },
        { { Input::Keyboard::OS::g_key_left_ctrl, Input::Keyboard::Key::Q }, AppAction::CloseApp            },
    };
    
    AppController(AppBase& application, const std::string& application_help,
                  const ActionByKeyboardState& action_by_keyboard_state = default_action_by_keyboard_state);
    
    // Input::Controller implementation
    void OnKeyboardChanged(Input::Keyboard::Key, Input::Keyboard::KeyState, const Input::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;

protected:
    // Input::Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(AppAction, Input::Keyboard::KeyState) override { /* not handled in this controller */ }
    void        OnKeyboardStateAction(AppAction action) override;
    std::string GetKeyboardActionName(AppAction action) const override;

private:
    AppBase& m_application;
};

} // namespace Methane::Platform
