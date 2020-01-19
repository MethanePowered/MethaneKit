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

FILE: Methane/Graphics/AppCameraController.cpp
Action camera controller with keyboard and mouse interactions handling.

******************************************************************************/

#include <Methane/Graphics/AppCameraController.h>
#include <Methane/Instrumentation.h>

using namespace Methane::Platform;

namespace Methane::Graphics
{

AppCameraController::AppCameraController(ActionCamera& action_camera, const std::string& camera_name,
                                         const ActionByMouseButton&   mouse_actions_by_button,
                                         const ActionByKeyboardState& keyboard_actions_by_state,
                                         const ActionByKeyboardKey&   keyboard_actions_by_key)
    : Controller(camera_name)
    , Mouse::ActionControllerBase<ActionCamera::MouseAction>(mouse_actions_by_button)
    , Keyboard::ActionControllerBase<ActionCamera::KeyboardAction>(keyboard_actions_by_state, keyboard_actions_by_key)
    , m_action_camera(action_camera)
{
    ITT_FUNCTION_TASK();
}

void AppCameraController::OnMouseButtonChanged(Platform::Mouse::Button button, Platform::Mouse::ButtonState button_state, const Platform::Mouse::StateChange& state_change)
{
    ITT_FUNCTION_TASK();
    const ActionCamera::MouseAction action = GetMouseActionByButton(button);
    switch (button_state)
    {
    case Platform::Mouse::ButtonState::Pressed:  m_action_camera.OnMousePressed(state_change.current.GetPosition(), action); break;
    case Platform::Mouse::ButtonState::Released: m_action_camera.OnMouseReleased(state_change.current.GetPosition()); break;
    }
}

void AppCameraController::OnMousePositionChanged(const Platform::Mouse::Position& mouse_position, const Platform::Mouse::StateChange&)
{
    ITT_FUNCTION_TASK();
    m_action_camera.OnMouseDragged(mouse_position);
}

void AppCameraController::OnMouseScrollChanged(const Platform::Mouse::Scroll& mouse_scroll_delta, const Platform::Mouse::StateChange&)
{
    ITT_FUNCTION_TASK();
    const auto mouse_button_and_delta = Platform::Mouse::GetScrollButtonAndDelta(mouse_scroll_delta);
    const ActionCamera::MouseAction action = GetMouseActionByButton(mouse_button_and_delta.first);
    if (action == ActionCamera::MouseAction::Zoom)
    {
        m_action_camera.OnMouseScrolled(mouse_button_and_delta.second);
    }
}

void AppCameraController::OnKeyboardChanged(Platform::Keyboard::Key key, Platform::Keyboard::KeyState key_state, const Platform::Keyboard::StateChange& state_change)
{
    ITT_FUNCTION_TASK();
    Keyboard::ActionControllerBase<ActionCamera::KeyboardAction>::OnKeyboardChanged(key, key_state, state_change);
}

AppCameraController::HelpLines AppCameraController::GetHelp() const
{
    ITT_FUNCTION_TASK();
    HelpLines help_lines;
    help_lines.reserve(m_action_by_mouse_button.size() + m_action_by_keyboard_key.size() + m_action_by_keyboard_state.size() + 2);

    const HelpLines mouse_help_lines = GetMouseHelp();
    if (!mouse_help_lines.empty())
    {
        help_lines.push_back({ "", "Mouse actions" });
        help_lines.insert(help_lines.end(), mouse_help_lines.begin(), mouse_help_lines.end());
    }

    const HelpLines keyboard_help_lines = GetKeyboardHelp();
    if (!keyboard_help_lines.empty())
    {
        help_lines.push_back({ "", "Keyboard actions" });
        help_lines.insert(help_lines.end(), keyboard_help_lines.begin(), keyboard_help_lines.end());
    }
    
    return help_lines;
}

void AppCameraController::OnKeyboardKeyAction(ActionCamera::KeyboardAction action, Platform::Keyboard::KeyState key_state)
{
    ITT_FUNCTION_TASK();
    switch (key_state)
    {
        case Platform::Keyboard::KeyState::Pressed:  m_action_camera.OnKeyPressed(action); break;
        case Platform::Keyboard::KeyState::Released: m_action_camera.OnKeyReleased(action); break;
    }
}

void AppCameraController::OnKeyboardStateAction(ActionCamera::KeyboardAction action)
{
    ITT_FUNCTION_TASK();
    m_action_camera.DoKeyboardAction(action);
}

std::string AppCameraController::GetKeyboardActionName(ActionCamera::KeyboardAction action) const
{
    ITT_FUNCTION_TASK();
    return ActionCamera::GetActionName(action);
}

std::string AppCameraController::GetMouseActionName(ActionCamera::MouseAction action) const
{
    ITT_FUNCTION_TASK();
    return ActionCamera::GetActionName(action);
}

} // namespace Methane::Graphics