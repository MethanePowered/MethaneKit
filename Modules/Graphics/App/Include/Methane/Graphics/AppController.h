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

FILE: Methane/Graphics/AppController.h
Base graphics application controller.

******************************************************************************/

#pragma once

#include "IApp.h"

#include <Methane/Platform/AppController.h>
#include <Methane/Platform/Input/KeyboardActionControllerBase.hpp>

namespace Methane::Graphics
{

enum class AppAction : uint32_t
{
    None = 0U,
    SwitchAnimations
};

namespace pin = Methane::Platform::Input;

class AppController
    : public Platform::AppController
    , public pin::Keyboard::ActionControllerBase<AppAction>
{
public:
    using ActionByKeyboardState = pin::Keyboard::ActionControllerBase<AppAction>::ActionByKeyboardState;
    inline static const ActionByKeyboardState default_action_by_keyboard_state{
        { { pin::Keyboard::Key::LeftControl, pin::Keyboard::Key::P }, AppAction::SwitchAnimations  }
    };
    
    AppController(IApp& application, const std::string& application_help,
                  const Platform::AppController::ActionByKeyboardState& platform_action_by_keyboard_state = Platform::AppController::default_action_by_keyboard_state,
                  const Graphics::AppController::ActionByKeyboardState& graphics_action_by_keyboard_state = Graphics::AppController::default_action_by_keyboard_state);
    
    // Input::Controller implementation
    void OnKeyboardChanged(pin::Keyboard::Key, pin::Keyboard::KeyState, const pin::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;

protected:
    using Platform::AppController::OnKeyboardKeyAction;
    using Platform::AppController::OnKeyboardStateAction;
    using Platform::AppController::GetKeyboardActionName;
    
    // Input::Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(AppAction, pin::Keyboard::KeyState) override { /* not handled in this controller */ }
    void        OnKeyboardStateAction(AppAction action) override;
    std::string GetKeyboardActionName(AppAction action) const override;

private:
    IApp& m_application;
};

} // namespace Methane::Platform
