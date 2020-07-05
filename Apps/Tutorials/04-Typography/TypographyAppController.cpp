/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: TypographyAppController.cpp
Typography application controller.

******************************************************************************/

#include "TypographyAppController.h"
#include "TypographyApp.h"

namespace Methane::Tutorials
{

TypographyAppController::TypographyAppController(TypographyApp& typography_app, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("TYPOGRAPHY SETTINGS")
    , Platform::Keyboard::ActionControllerBase<TypographyAppAction>(action_by_keyboard_state, {})
    , m_typography_app(typography_app)
{
    META_FUNCTION_TASK();
}

void TypographyAppController::OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state,
                                               const Platform::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Platform::Keyboard::ActionControllerBase<TypographyAppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void TypographyAppController::OnKeyboardStateAction(TypographyAppAction action)
{
    META_FUNCTION_TASK();
    
    switch(action)
    {
    case TypographyAppAction::ShowParameters:
        break;

    case TypographyAppAction::SwitchIncrementalTextUpdate:
        m_typography_app.SetIncrementalTextUpdate(!m_typography_app.IsIncrementalTextUpdate());
        break;

    default:
        assert(0);
    }
}

std::string TypographyAppController::GetKeyboardActionName(TypographyAppAction action) const
{
    META_FUNCTION_TASK();
    switch(action)
    {
    case TypographyAppAction::ShowParameters:               return "show text parameters";
    case TypographyAppAction::SwitchIncrementalTextUpdate:  return "switch incremental text update";
    default: assert(0);
    }
    return "";
}

Platform::Input::IHelpProvider::HelpLines TypographyAppController::GetHelp() const
{
    META_FUNCTION_TASK();
    return GetKeyboardHelp();
}

} // namespace Methane::Graphics
