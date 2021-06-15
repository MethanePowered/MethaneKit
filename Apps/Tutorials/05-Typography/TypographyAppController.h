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

FILE: TypographyAppController.h
Typography application controller.

******************************************************************************/

#pragma once

#include <Methane/Platform/Input/Controller.h>
#include <Methane/Platform/KeyboardActionControllerBase.hpp>

namespace Methane::Tutorials
{

class TypographyApp;

enum class TypographyAppAction
{
    None,
    SwitchTextWrapMode,
    SwitchTextHorizontalAlignment,
    SwitchTextVerticalAlignment,
    SwitchIncrementalTextUpdate,
    SwitchTypingDirection,
    SpeedupTyping,
    SlowdownTyping
};

class TypographyAppController final
    : public Platform::Input::Controller
    , public Platform::Keyboard::ActionControllerBase<TypographyAppAction>
{
public:
    TypographyAppController(TypographyApp& asteroids_app, const ActionByKeyboardState& action_by_keyboard_state);

    // Input::Controller implementation
    void OnKeyboardChanged(Platform::Keyboard::Key, Platform::Keyboard::KeyState, const Platform::Keyboard::StateChange& state_change) override;
    HelpLines GetHelp() const override;
    
protected:
    // Keyboard::ActionControllerBase interface
    void        OnKeyboardKeyAction(TypographyAppAction, Platform::Keyboard::KeyState) override { /* not handled in this controller */ }
    void        OnKeyboardStateAction(TypographyAppAction action) override;
    std::string GetKeyboardActionName(TypographyAppAction action) const override;

private:
    TypographyApp& m_typography_app;
};

} // namespace Methane::Graphics
