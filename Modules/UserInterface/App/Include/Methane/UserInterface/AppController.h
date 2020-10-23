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

FILE: Methane/UserInterface/AppController.h
Base user interface application controller.

******************************************************************************/

#pragma once

#include "App.h"

#include <Methane/Graphics/AppController.h>

namespace Methane::UserInterface
{

enum class AppAction : uint32_t
{
    None = 0,

    SwitchHeadsUpDisplayMode,

    Count
};

class AppController
    : public Graphics::AppController
    , public Platform::Keyboard::ActionControllerBase<AppAction>
{
public:
    using ActionByKeyboardState = Platform::Keyboard::ActionControllerBase<AppAction>::ActionByKeyboardState;
    inline static const ActionByKeyboardState default_action_by_keyboard_state{
        { { Platform::Keyboard::Key::F4 }, AppAction::SwitchHeadsUpDisplayMode },
    };
    
    AppController(IApp& application, const std::string& application_help,
                  const Platform::AppController::ActionByKeyboardState& platform_action_by_keyboard_state = Platform::AppController::default_action_by_keyboard_state,
                  const Graphics::AppController::ActionByKeyboardState& graphics_action_by_keyboard_state = Graphics::AppController::default_action_by_keyboard_state,
                  const UserInterface::AppController::ActionByKeyboardState& ui_action_by_keyboard_state  = UserInterface::AppController::default_action_by_keyboard_state);
    
    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key, Platform::Keyboard::KeyState, const Platform::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;

protected:
    using Graphics::AppController::OnKeyboardKeyAction;
    using Graphics::AppController::OnKeyboardStateAction;
    using Graphics::AppController::GetKeyboardActionName;

    // Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(AppAction, Platform::Keyboard::KeyState) override { }
    void        OnKeyboardStateAction(AppAction action) override;
    std::string GetKeyboardActionName(AppAction action) const override;

private:
    IApp& m_application;
};

} // namespace Methane::UserInterface
