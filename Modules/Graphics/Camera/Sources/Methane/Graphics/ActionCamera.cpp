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
#include <Methane/Data/TimeAnimation.hpp>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

ActionCamera::ActionCamera(Data::AnimationsPool& animations, Pivot pivot) noexcept
    : ArcBallCamera(pivot)
    , m_animations(animations)
{ }

ActionCamera::ActionCamera(const Camera& view_camera, Data::AnimationsPool& animations, Pivot pivot) noexcept
    : ArcBallCamera(view_camera, pivot)
    , m_animations(animations)
{ }

void ActionCamera::OnMousePressed(const Data::Point2I& mouse_screen_pos, MouseAction mouse_action) noexcept
{
    META_FUNCTION_TASK();
    m_mouse_action = mouse_action;
    SetMousePressedOrientation(GetOrientation());

    switch (m_mouse_action)
    {
    case MouseAction::Rotate:
        ArcBallCamera::MousePress(mouse_screen_pos);
        break;

    case MouseAction::Move:
        m_mouse_pressed_in_world = GetViewCamera().TransformScreenToWorld(mouse_screen_pos);
        break;

    default:
        return;
    }
}

void ActionCamera::OnMouseDragged(const Data::Point2I& mouse_screen_pos)
{
    META_FUNCTION_TASK();
    switch (m_mouse_action)
    {
    case MouseAction::Rotate:
        ArcBallCamera::MouseDrag(mouse_screen_pos);
        break;

    case MouseAction::Move:
        Move(GetViewCamera().TransformScreenToWorld(mouse_screen_pos) - m_mouse_pressed_in_world);
        break;

    default:
        return;
    }
}

void ActionCamera::OnMouseReleased(const Data::Point2I&) noexcept
{
    META_FUNCTION_TASK();
    m_mouse_action = MouseAction::None;
}

void ActionCamera::OnMouseScrolled(float scroll_delta)
{
    META_FUNCTION_TASK();
    using enum KeyboardAction;
    const KeyboardAction zoom_action = scroll_delta > 0.F ? ZoomIn : ZoomOut;
    const float          zoom_factor = scroll_delta > 0.F
                                     ? 1.F - scroll_delta / static_cast<float>(m_zoom_steps_count)
                                     : 1.F / (1.F + scroll_delta / static_cast<float>(m_zoom_steps_count));
    
    StopKeyboardAction(zoom_action == ZoomIn ? ZoomOut : ZoomIn, 0.0);
    StartZoomAction(zoom_action, zoom_factor, m_keyboard_action_duration_sec);
}

void ActionCamera::OnKeyPressed(KeyboardAction keyboard_action)
{
    META_FUNCTION_TASK();
    const float rotation_axis_sign = GetPivot() == Pivot::Aim ? 1.F : -1.F;

    switch(keyboard_action)
    {
        using enum KeyboardAction;

        // Move
        case MoveLeft:    StartMoveAction(keyboard_action,   hlslpp::float3(-1.F,  0.F,  0.F)); break;
        case MoveRight:   StartMoveAction(keyboard_action,   hlslpp::float3( 1.F,  0.F,  0.F)); break;
        case MoveForward: StartMoveAction(keyboard_action,   hlslpp::float3( 0.F,  0.F,  1.F)); break;
        case MoveBack:    StartMoveAction(keyboard_action,   hlslpp::float3( 0.F,  0.F, -1.F)); break;
        case MoveUp:      StartMoveAction(keyboard_action,   hlslpp::float3( 0.F,  1.F,  0.F)); break;
        case MoveDown:    StartMoveAction(keyboard_action,   hlslpp::float3( 0.F, -1.F,  0.F)); break;
            
        // Rotate
        case YawLeft:     StartRotateAction(keyboard_action, hlslpp::float3( 0.F, -1.F,  0.F) * rotation_axis_sign); break;
        case YawRight:    StartRotateAction(keyboard_action, hlslpp::float3( 0.F,  1.F,  0.F) * rotation_axis_sign); break;
        case RollLeft:    StartRotateAction(keyboard_action, hlslpp::float3( 0.F,  0.F,  1.F) * rotation_axis_sign); break;
        case RollRight:   StartRotateAction(keyboard_action, hlslpp::float3( 0.F,  0.F, -1.F) * rotation_axis_sign); break;
        case PitchUp:     StartRotateAction(keyboard_action, hlslpp::float3(-1.F,  0.F,  0.F) * rotation_axis_sign); break;
        case PitchDown:   StartRotateAction(keyboard_action, hlslpp::float3( 1.F,  0.F,  0.F) * rotation_axis_sign); break;
            
        // Zoom
        case ZoomIn:      StartZoomAction(keyboard_action, 0.9F); break;
        case ZoomOut:     StartZoomAction(keyboard_action, 1.1F); break;
            
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
        using enum KeyboardAction;
        using enum Pivot;
        case Reset:       ResetOrientation(); break;
        case ChangePivot: SetPivot(GetPivot() == Aim ? Eye : Aim); break;
        default:          return;
    }
}

void ActionCamera::Move(const hlslpp::float3& move_vector) noexcept
{
    META_FUNCTION_TASK();
    SetOrientationAim(GetOrientation().aim + move_vector);
    SetOrientationEye(GetOrientation().eye + move_vector);
    Camera::LogOrientation();
}

void ActionCamera::Zoom(float zoom_factor) noexcept
{
    META_FUNCTION_TASK();
    const hlslpp::float3 look_dir   = GetLookDirection(GetOrientation());
    const float zoom_distance = std::min(std::max(static_cast<float>(hlslpp::length(look_dir)) * zoom_factor, m_zoom_distance_range.first), m_zoom_distance_range.second);
    ApplyLookDirection(hlslpp::normalize(look_dir) * zoom_distance);
    Camera::LogOrientation();
}

void ActionCamera::StartRotateAction(KeyboardAction rotate_action, const hlslpp::float3& rotation_axis_in_view, double duration_sec)
{
    META_FUNCTION_TASK();
    if (StartKeyboardAction(rotate_action, duration_sec))
        return;
    
    const float angle_rad_per_second = Methane::Data::DegreeToRadians(m_rotate_angle_per_second);
    m_animations.push_back(
        Data::MakeTimeAnimationPtr([this, angle_rad_per_second, rotation_axis_in_view](double elapsed_seconds, double delta_seconds)
        {
            RotateInView(rotation_axis_in_view, static_cast<float>(angle_rad_per_second * delta_seconds * GetAccelerationFactor(elapsed_seconds)));
            return true;
        }, duration_sec)
    );

    const bool animation_added = m_keyboard_action_animations.try_emplace(rotate_action, m_animations.back()).second;
    META_CHECK_TRUE(animation_added);
}

void ActionCamera::StartMoveAction(KeyboardAction move_action, const hlslpp::float3& move_direction_in_view, double duration_sec)
{
    META_FUNCTION_TASK();
    if (StartKeyboardAction(move_action, duration_sec))
        return;

    m_animations.push_back(
        Data::MakeTimeAnimationPtr([this, move_direction_in_view](double elapsed_seconds, double delta_seconds)
        {
            const hlslpp::float3 move_per_second = hlslpp::normalize(TransformViewToWorld(move_direction_in_view)) * m_move_distance_per_second;
            Move(move_per_second * delta_seconds * GetAccelerationFactor(elapsed_seconds));
            return true;
        }, duration_sec)
    );

    const bool animation_added = m_keyboard_action_animations.try_emplace(move_action, m_animations.back()).second;
    META_CHECK_TRUE(animation_added);
}

void ActionCamera::StartZoomAction(KeyboardAction zoom_action, float zoom_factor_per_second, double duration_sec)
{
    META_FUNCTION_TASK();
    if (StartKeyboardAction(zoom_action, duration_sec))
        return;

    m_animations.push_back(
        Data::MakeTimeAnimationPtr([this, zoom_factor_per_second](double elapsed_seconds, double delta_seconds)
        {
            Zoom(1.F - static_cast<float>((1.F - zoom_factor_per_second) * delta_seconds * GetAccelerationFactor(elapsed_seconds)));
            return true;
        }, duration_sec)
    );

    const bool animation_added = m_keyboard_action_animations.try_emplace(zoom_action, m_animations.back()).second;
    META_CHECK_TRUE(animation_added);
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
    default:                    META_UNEXPECTED_RETURN(mouse_action, "");
    }
}

std::string ActionCamera::GetActionName(KeyboardAction keyboard_action)
{
    META_FUNCTION_TASK();
    switch (keyboard_action)
    {
    using enum KeyboardAction;

    // Move
    case MoveLeft:    return "move left";
    case MoveRight:   return "move right";
    case MoveForward: return "move forward";
    case MoveBack:    return "move backward";
    case MoveUp:      return "move up";
    case MoveDown:    return "move down";

    // Rotate
    case YawLeft:     return "yaw left";
    case YawRight:    return "yaw right";
    case RollLeft:    return "roll left";
    case RollRight:   return "roll right";
    case PitchUp:     return "pitch up";
    case PitchDown:   return "pitch down";

    // Zoom
    case ZoomIn:      return "zoom in";
    case ZoomOut:     return "zoom out";

    // Other
    case Reset:       return "reset orientation";
    case ChangePivot: return "change pivot";

    case None:        return "none";
    default:          META_UNEXPECTED_RETURN(keyboard_action, "");
    }
}

} // namespace Methane::Graphics
