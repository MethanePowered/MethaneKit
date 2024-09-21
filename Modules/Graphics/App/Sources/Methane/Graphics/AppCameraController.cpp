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

FILE: Methane/Graphics/AppCameraController.cpp
Action camera controller with keyboard and mouse interactions handling.

******************************************************************************/

#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

AppCameraController::AppCameraController(ActionCamera& action_camera, const std::string& camera_name,
                                         const ActionByMouseButton&   mouse_actions_by_button,
                                         const ActionByKeyboardState& keyboard_actions_by_state,
                                         const ActionByKeyboardKey&   keyboard_actions_by_key)
    : Controller(camera_name)
    , pin::Mouse::ActionControllerBase<ActionCamera::MouseAction>(mouse_actions_by_button)
    , pin::Keyboard::ActionControllerBase<ActionCamera::KeyboardAction>(keyboard_actions_by_state, keyboard_actions_by_key)
    , m_action_camera(action_camera)
{ }

void AppCameraController::OnMouseButtonChanged(pin::Mouse::Button button, pin::Mouse::ButtonState button_state, const pin::Mouse::StateChange& state_change)
{
    META_FUNCTION_TASK();
    const ActionCamera::MouseAction action = GetMouseActionByButton(button);
    switch (button_state)
    {
    case pin::Mouse::ButtonState::Pressed:  m_action_camera.OnMousePressed(state_change.current.GetPosition(), action); break;
    case pin::Mouse::ButtonState::Released: m_action_camera.OnMouseReleased(state_change.current.GetPosition()); break;
    default: META_UNEXPECTED(button_state);
    }
}

void AppCameraController::OnMousePositionChanged(const pin::Mouse::Position& mouse_position, const pin::Mouse::StateChange&)
{
    META_FUNCTION_TASK();
    m_action_camera.OnMouseDragged(mouse_position);
}

void AppCameraController::OnMouseScrollChanged(const pin::Mouse::Scroll& mouse_scroll_delta, const pin::Mouse::StateChange&)
{
    META_FUNCTION_TASK();
    const auto [mouse_button, scroll_delta] = pin::Mouse::GetScrollButtonAndDelta(mouse_scroll_delta);
    if (const ActionCamera::MouseAction action = GetMouseActionByButton(mouse_button);
        action == ActionCamera::MouseAction::Zoom)
    {
        m_action_camera.OnMouseScrolled(scroll_delta);
    }
}

void AppCameraController::OnKeyboardChanged(pin::Keyboard::Key key, pin::Keyboard::KeyState key_state, const pin::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    pin::Keyboard::ActionControllerBase<ActionCamera::KeyboardAction>::OnKeyboardChanged(key, key_state, state_change);
}

AppCameraController::HelpLines AppCameraController::GetHelp() const
{
    META_FUNCTION_TASK();
    HelpLines help_lines;
    help_lines.reserve(GetMouseActionsCount() + GetKeyboardActionsCount() + 2);

    if (const HelpLines mouse_help_lines = GetMouseHelp();
        !mouse_help_lines.empty())
    {
        help_lines.emplace_back("", "Mouse actions");
        help_lines.insert(help_lines.end(), mouse_help_lines.begin(), mouse_help_lines.end());
    }

    if (const HelpLines keyboard_help_lines = GetKeyboardHelp();
        !keyboard_help_lines.empty())
    {
        help_lines.emplace_back("", "Keyboard actions");
        help_lines.insert(help_lines.end(), keyboard_help_lines.begin(), keyboard_help_lines.end());
    }
    
    return help_lines;
}

void AppCameraController::OnKeyboardKeyAction(ActionCamera::KeyboardAction action, pin::Keyboard::KeyState key_state)
{
    META_FUNCTION_TASK();
    switch (key_state)
    {
    case pin::Keyboard::KeyState::Pressed:  m_action_camera.OnKeyPressed(action); break;
    case pin::Keyboard::KeyState::Released: m_action_camera.OnKeyReleased(action); break;
    default: META_UNEXPECTED(key_state);
    }
}

void AppCameraController::OnKeyboardStateAction(ActionCamera::KeyboardAction action)
{
    META_FUNCTION_TASK();
    m_action_camera.DoKeyboardAction(action);
}

std::string AppCameraController::GetKeyboardActionName(ActionCamera::KeyboardAction action) const
{
    META_FUNCTION_TASK();
    return ActionCamera::GetActionName(action);
}

std::string AppCameraController::GetMouseActionName(ActionCamera::MouseAction action) const
{
    META_FUNCTION_TASK();
    return ActionCamera::GetActionName(action);
}

} // namespace Methane::Graphics