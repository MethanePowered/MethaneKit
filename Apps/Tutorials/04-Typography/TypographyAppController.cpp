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
    constexpr double s_text_update_interval_delta = 0.01;
    gui::Text::Layout text_layout = m_typography_app.GetSettings().text_layout;

    switch(action)
    {
    case TypographyAppAction::SwitchTextWrapMode:
        text_layout.wrap = static_cast<gui::Text::Wrap>((static_cast<uint32_t>(text_layout.wrap) + 1) % 3);
        m_typography_app.SetTextLayout(text_layout);
        break;

    case TypographyAppAction::SwitchTextHorizontalAlignment:
        text_layout.horizontal_alignment = static_cast<gui::Text::HorizontalAlignment>((static_cast<uint32_t>(text_layout.horizontal_alignment) + 1) % 3);
        m_typography_app.SetTextLayout(text_layout);
        break;

    case TypographyAppAction::SwitchTextVerticalAlignment:
        text_layout.vertical_alignment = static_cast<gui::Text::VerticalAlignment>((static_cast<uint32_t>(text_layout.vertical_alignment) + 1) % 3);
        m_typography_app.SetTextLayout(text_layout);
        break;

    case TypographyAppAction::SwitchIncrementalTextUpdate:
        m_typography_app.SetIncrementalTextUpdate(!m_typography_app.GetSettings().is_incremental_text_update);
        break;

    case TypographyAppAction::SwitchTypingDirection:
        m_typography_app.SetForwardTypingDirection(!m_typography_app.GetSettings().is_forward_typing_direction);
        break;

    case TypographyAppAction::SpeedupTyping:
        m_typography_app.SetTextUpdateInterval(
            std::max(s_text_update_interval_delta, m_typography_app.GetSettings().typing_update_interval_sec - s_text_update_interval_delta));
        break;

    case TypographyAppAction::SlowdownTyping:
        m_typography_app.SetTextUpdateInterval( m_typography_app.GetSettings().typing_update_interval_sec + s_text_update_interval_delta);
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
    case TypographyAppAction::SwitchTextWrapMode:            return "switch text wrap mode";
    case TypographyAppAction::SwitchTextHorizontalAlignment: return "switch horizontal text alignment";
    case TypographyAppAction::SwitchTextVerticalAlignment:   return "switch vertical text alignment";
    case TypographyAppAction::SwitchIncrementalTextUpdate:   return "switch incremental text update";
    case TypographyAppAction::SwitchTypingDirection:         return "switch typing direction";
    case TypographyAppAction::SpeedupTyping:                 return "speedup typing";
    case TypographyAppAction::SlowdownTyping:                return "slowdown typing";
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
