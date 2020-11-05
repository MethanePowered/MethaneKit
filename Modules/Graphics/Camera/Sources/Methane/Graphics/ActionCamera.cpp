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

FILE: Methane/Graphics/ActionCamera.cpp
Interactive action-camera for rotating, moving and zooming with mouse and keyboard.

******************************************************************************/

#include <Methane/Graphics/ActionCamera.h>
#include <Methane/Data/TimeAnimation.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <cml/mathlib/mathlib.h>

namespace Methane::Graphics
{

ActionCamera::ActionCamera(Data::AnimationsPool& animations, Pivot pivot, cml::AxisOrientation axis_orientation) noexcept
    : ArcBallCamera(pivot, axis_orientation)
    , m_animations(animations)
{
    META_FUNCTION_TASK();
}

ActionCamera::ActionCamera(const Camera& view_camera, Data::AnimationsPool& animations, Pivot pivot, cml::AxisOrientation axis_orientation) noexcept
    : ArcBallCamera(view_camera, pivot, axis_orientation)
    , m_animations(animations)
{
    META_FUNCTION_TASK();
}

void ActionCamera::OnMousePressed(const Data::Point2i& mouse_screen_pos, MouseAction mouse_action) noexcept
{
    META_FUNCTION_TASK();
    m_mouse_action = mouse_action;
    SetMousePressedOrientation(GetOrientation());

    switch (m_mouse_action)
    {
    case MouseAction::Rotate:
        ArcBallCamera::OnMousePressed(mouse_screen_pos);
        break;

    case MouseAction::Move:
        m_mouse_pressed_in_world = GetViewCamera().TransformScreenToWorld(mouse_screen_pos);
        break;

    default:
        return;
    }
}

void ActionCamera::OnMouseDragged(const Data::Point2i& mouse_screen_pos) noexcept
{
    META_FUNCTION_TASK();
    switch (m_mouse_action)
    {
    case MouseAction::Rotate:
        ArcBallCamera::OnMouseDragged(mouse_screen_pos);
        break;

    case MouseAction::Move:
        Move(GetViewCamera().TransformScreenToWorld(mouse_screen_pos) - m_mouse_pressed_in_world);
        break;

    default:
        return;
    }
}

void ActionCamera::OnMouseReleased(const Data::Point2i&) noexcept
{
    META_FUNCTION_TASK();
    m_mouse_action = MouseAction::None;
}

void ActionCamera::OnMouseScrolled(float scroll_delta)
{
    META_FUNCTION_TASK();
    const KeyboardAction zoom_action = scroll_delta > 0.F
                                     ? KeyboardAction::ZoomIn : KeyboardAction::ZoomOut;
    const float          zoom_factor = scroll_delta > 0.F
                                     ? 1.F - scroll_delta / m_zoom_steps_count
                                     : 1.F / (1.F + scroll_delta / m_zoom_steps_count);
    
    StopKeyboardAction(zoom_action == KeyboardAction::ZoomIn ? KeyboardAction::ZoomOut : KeyboardAction::ZoomIn, 0.0);
    StartZoomAction(zoom_action, zoom_factor, m_keyboard_action_duration_sec);
}

void ActionCamera::OnKeyPressed(KeyboardAction keyboard_action)
{
    META_FUNCTION_TASK();
    const float rotation_axis_sign = GetPivot() == Pivot::Aim ? 1.F : -1.F;

    switch(keyboard_action)
    {
        // Move
        case KeyboardAction::MoveLeft:      StartMoveAction(keyboard_action,   Vector3f(-1.F,  0.F,  0.F)); break;
        case KeyboardAction::MoveRight:     StartMoveAction(keyboard_action,   Vector3f( 1.F,  0.F,  0.F)); break;
        case KeyboardAction::MoveForward:   StartMoveAction(keyboard_action,   Vector3f( 0.F,  0.F,  1.F)); break;
        case KeyboardAction::MoveBack:      StartMoveAction(keyboard_action,   Vector3f( 0.F,  0.F, -1.F)); break;
        case KeyboardAction::MoveUp:        StartMoveAction(keyboard_action,   Vector3f( 0.F,  1.F,  0.F)); break;
        case KeyboardAction::MoveDown:      StartMoveAction(keyboard_action,   Vector3f( 0.F, -1.F,  0.F)); break;
            
        // Rotate
        case KeyboardAction::YawLeft:       StartRotateAction(keyboard_action, Vector3f( 0.F, -1.F,  0.F) * rotation_axis_sign); break;
        case KeyboardAction::YawRight:      StartRotateAction(keyboard_action, Vector3f( 0.F,  1.F,  0.F) * rotation_axis_sign); break;
        case KeyboardAction::RollLeft:      StartRotateAction(keyboard_action, Vector3f( 0.F,  0.F,  1.F) * rotation_axis_sign); break;
        case KeyboardAction::RollRight:     StartRotateAction(keyboard_action, Vector3f( 0.F,  0.F, -1.F) * rotation_axis_sign); break;
        case KeyboardAction::PitchUp:       StartRotateAction(keyboard_action, Vector3f(-1.F,  0.F,  0.F) * rotation_axis_sign); break;
        case KeyboardAction::PitchDown:     StartRotateAction(keyboard_action, Vector3f( 1.F,  0.F,  0.F) * rotation_axis_sign); break;
            
        // Zoom
        case KeyboardAction::ZoomIn:        StartZoomAction(keyboard_action, 0.9F); break;
        case KeyboardAction::ZoomOut:       StartZoomAction(keyboard_action, 1.1F); break;
            
        default: return;
    }
}

void ActionCamera::OnKeyReleased(KeyboardAction keyboard_action)
{
    META_FUNCTION_TASK();
    StopKeyboardAction(keyboard_action, m_keyboard_action_duration_sec);
}

void ActionCamera::DoKeyboardAction(KeyboardAction keyboard_action) noexcept
{
    META_FUNCTION_TASK();
    switch(keyboard_action)
    {
        case KeyboardAction::Reset:
            ResetOrientation();
            break;

        case KeyboardAction::ChangePivot:
            SetPivot(GetPivot() == Pivot::Aim ? Pivot::Eye : Pivot::Aim);
            break;
        
        default:
            return;
    }
}

void ActionCamera::Move(const Vector3f& move_vector) noexcept
{
    META_FUNCTION_TASK();
    SetOrientationAim(GetOrientation().aim + move_vector);
    SetOrientationEye(GetOrientation().eye + move_vector);
    META_LOG(GetOrientationString());
}

void ActionCamera::Zoom(float zoom_factor) noexcept
{
    META_FUNCTION_TASK();
    const Vector3f look_dir   = GetLookDirection(GetOrientation());
    const float zoom_distance = std::min(std::max(look_dir.length() * zoom_factor, m_zoom_distance_range.first), m_zoom_distance_range.second);
    ApplyLookDirection(cml::normalize(look_dir) * zoom_distance);
    META_LOG(GetOrientationString());
}

void ActionCamera::StartRotateAction(KeyboardAction rotate_action, const Vector3f& rotation_axis_in_view, double duration_sec)
{
    META_FUNCTION_TASK();
    if (StartKeyboardAction(rotate_action, duration_sec))
        return;
    
    const float angle_rad_per_second = cml::rad(m_rotate_angle_per_second);
    m_animations.push_back(
        std::make_shared<Data::TimeAnimation>([this, angle_rad_per_second, rotation_axis_in_view](double elapsed_seconds, double delta_seconds)
            {
                RotateInView(rotation_axis_in_view, static_cast<float>(angle_rad_per_second * delta_seconds * GetAccelerationFactor(elapsed_seconds)));
                return true;
            },
            duration_sec));
    
    const auto emplace_result = m_keyboard_action_animations.emplace(rotate_action, m_animations.back());
    META_CHECK_ARG_TRUE(emplace_result.second);
}

void ActionCamera::StartMoveAction(KeyboardAction move_action, const Vector3f& move_direction_in_view, double duration_sec)
{
    META_FUNCTION_TASK();
    if (StartKeyboardAction(move_action, duration_sec))
        return;
    
    m_animations.push_back(
        std::make_shared<Data::TimeAnimation>([this, move_direction_in_view](double elapsed_seconds, double delta_seconds)
            {
                const Vector3f move_per_second = TransformViewToWorld(move_direction_in_view).normalize() * m_move_distance_per_second;
                Move(move_per_second * delta_seconds * GetAccelerationFactor(elapsed_seconds));
                return true;
            },
            duration_sec)
    );
    
    const auto emplace_result = m_keyboard_action_animations.emplace(move_action, m_animations.back());
    META_CHECK_ARG_TRUE(emplace_result.second);
}

void ActionCamera::StartZoomAction(KeyboardAction zoom_action, float zoom_factor_per_second, double duration_sec)
{
    META_FUNCTION_TASK();
    if (StartKeyboardAction(zoom_action, duration_sec))
        return;
    
    m_animations.push_back(
        std::make_shared<Data::TimeAnimation>([this, zoom_factor_per_second](double elapsed_seconds, double delta_seconds)
            {
                Zoom(1.F - static_cast<float>((1.F - zoom_factor_per_second) * delta_seconds * GetAccelerationFactor(elapsed_seconds)));
                return true;
            },
            duration_sec)
    );
    
    const auto emplace_result = m_keyboard_action_animations.emplace(zoom_action, m_animations.back());
    META_CHECK_ARG_TRUE(emplace_result.second);
}

bool ActionCamera::StartKeyboardAction(KeyboardAction keyboard_action, double duration_sec)
{
    META_FUNCTION_TASK();
    const auto keyboard_action_animations_it = m_keyboard_action_animations.find(keyboard_action);
    if (keyboard_action_animations_it == m_keyboard_action_animations.end())
        return false;
    
    if (keyboard_action_animations_it->second.expired())
    {
        m_keyboard_action_animations.erase(keyboard_action_animations_it);
        return false;
    }
    else
    {
        // Continue animation until key is released
        keyboard_action_animations_it->second.lock()->IncreaseDuration(duration_sec);
        return true;
    }
}

bool ActionCamera::StopKeyboardAction(KeyboardAction keyboard_action, double duration_sec)
{
    META_FUNCTION_TASK();
    const auto keyboard_action_animations_it = m_keyboard_action_animations.find(keyboard_action);
    if (keyboard_action_animations_it == m_keyboard_action_animations.end())
        return false;

    if (keyboard_action_animations_it->second.expired())
    {
        m_keyboard_action_animations.erase(keyboard_action_animations_it);
        return false;
    }
    else
    {
        // Stop animation in a fixed duration after it was started
        if (duration_sec > 0.0)
            keyboard_action_animations_it->second.lock()->SetDuration(duration_sec);
        else
            keyboard_action_animations_it->second.lock()->Stop();
        return true;
    }
}

std::string ActionCamera::GetActionName(MouseAction mouse_action)
{
    META_FUNCTION_TASK();
    switch (mouse_action)
    {
    case MouseAction::Rotate:   return "rotate";
    case MouseAction::Zoom:     return "zoom";
    case MouseAction::Move:     return "move";
    case MouseAction::None:     return "none";
    default:                    META_UNEXPECTED_ENUM_ARG_RETURN(mouse_action, "");
    }
}

std::string ActionCamera::GetActionName(KeyboardAction keyboard_action)
{
    META_FUNCTION_TASK();
    switch (keyboard_action)
    {
    // Move
    case KeyboardAction::MoveLeft:      return "move left";
    case KeyboardAction::MoveRight:     return "move right";
    case KeyboardAction::MoveForward:   return "move forward";
    case KeyboardAction::MoveBack:      return "move backward";
    case KeyboardAction::MoveUp:        return "move up";
    case KeyboardAction::MoveDown:      return "move down";

    // Rotate
    case KeyboardAction::YawLeft:       return "yaw left";
    case KeyboardAction::YawRight:      return "yaw right";
    case KeyboardAction::RollLeft:      return "roll left";
    case KeyboardAction::RollRight:     return "roll right";
    case KeyboardAction::PitchUp:       return "pitch up";
    case KeyboardAction::PitchDown:     return "pitch down";

    // Zoom
    case KeyboardAction::ZoomIn:        return "zoom in";
    case KeyboardAction::ZoomOut:       return "zoom out";

    // Other
    case KeyboardAction::Reset:         return "reset orientation";
    case KeyboardAction::ChangePivot:   return "change pivot";

    case KeyboardAction::None:          return "none";
    default:                            META_UNEXPECTED_ENUM_ARG_RETURN(keyboard_action, "");
    }
}

} // namespace Methane::Graphics
