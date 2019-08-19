/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ActionCamera.cpp
Interactive action-camera for rotating, moving and zooming with mouse and keyboard.
Implemented based on arc-ball camera rotation model.

******************************************************************************/

#include <Methane/Graphics/ActionCamera.h>
#include <Methane/Data/TimeAnimation.h>

#include <cml/mathlib/mathlib.h>

#include <cassert>

using namespace Methane::Data;
using namespace Methane::Graphics;

namespace Methane
{
static inline float square(float x)     { return x * x; }
static inline float unitSign(float x) { return x / std::fabsf(x); }
}

ActionCamera::ActionCamera(AnimationsPool& animations, Pivot pivot, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_animations(animations)
    , m_p_view_camera(nullptr)
    , m_pivot(pivot)
{
}

ActionCamera::ActionCamera(const Camera& view_camera, AnimationsPool& animations, Pivot pivot, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_animations(animations)
    , m_p_view_camera(&view_camera)
    , m_pivot(pivot)
{
}

void ActionCamera::OnMousePressed(const Point2i& mouse_screen_pos, MouseAction mouse_action)
{
    m_mouse_action = mouse_action;
    m_mouse_pressed_orientation = m_current_orientation;

    switch (m_mouse_action)
    {
    case MouseAction::Rotate:
        m_mouse_pressed_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, true);
        break;

    case MouseAction::Move:
        m_mouse_pressed_in_world = GetViewCamera().TransformScreenToWorld(mouse_screen_pos);
        break;

    default:
        break;
    }
}

void ActionCamera::OnMouseDragged(const Point2i& mouse_screen_pos)
{
    switch (m_mouse_action)
    {
    case MouseAction::Rotate:
    {
        const Vector3f mouse_current_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, false);
        const Vector3f vectors_cross = cml::cross(m_mouse_pressed_on_sphere, mouse_current_on_sphere);
        const Vector3f rotation_axis = vectors_cross.normalize();
        const float    rotation_angle = std::atan2f(vectors_cross.length(), cml::dot(m_mouse_pressed_on_sphere, mouse_current_on_sphere));

        Rotate(rotation_axis, rotation_angle, m_mouse_pressed_orientation);

        // NOTE: fixes rotation axis flip at angles approaching to 180 degrees
        if (std::abs(rotation_angle) > cml::rad(90.f))
        {
            m_mouse_pressed_orientation = m_current_orientation;
            m_mouse_pressed_on_sphere = mouse_current_on_sphere;
        }
    } break;

    case MouseAction::Move:
    {
        const Vector3f mouse_current_in_world = GetViewCamera().TransformScreenToWorld(mouse_screen_pos);
        Move(mouse_current_in_world - m_mouse_pressed_in_world);
    } break;

    default: return;
    }
}

void ActionCamera::OnMouseReleased(const Point2i&)
{
    m_mouse_action = MouseAction::None;
}

void ActionCamera::OnMouseScrolled(float scroll_delta)
{
    const KeyboardAction zoom_action = scroll_delta > 0.f
                                     ? KeyboardAction::ZoomIn : KeyboardAction::ZoomOut;
    const float          zoom_factor = scroll_delta > 0.f
                                     ? 1.f - scroll_delta / m_zoom_steps_count
                                     : 1.f / (1.f + scroll_delta / m_zoom_steps_count);
    
    StopKeyboardAction(zoom_action == KeyboardAction::ZoomIn ? KeyboardAction::ZoomOut : KeyboardAction::ZoomIn, 0.0);
    StartZoomAction(zoom_action, zoom_factor, m_keyboard_action_duration_sec);
}

void ActionCamera::OnKeyPressed(KeyboardAction keyboard_action)
{
    const float rotation_axis_sign = m_pivot == Pivot::Aim ? 1.f : -1.f;

    switch(keyboard_action)
    {
        // Move
        case KeyboardAction::MoveLeft:      StartMoveAction(keyboard_action,   Vector3f(-1.f,  0.f,  0.f)); break;
        case KeyboardAction::MoveRight:     StartMoveAction(keyboard_action,   Vector3f( 1.f,  0.f,  0.f)); break;
        case KeyboardAction::MoveForward:   StartMoveAction(keyboard_action,   Vector3f( 0.f,  0.f,  1.f)); break;
        case KeyboardAction::MoveBack:      StartMoveAction(keyboard_action,   Vector3f( 0.f,  0.f, -1.f)); break;
        case KeyboardAction::MoveUp:        StartMoveAction(keyboard_action,   Vector3f( 0.f,  1.f,  0.f)); break;
        case KeyboardAction::MoveDown:      StartMoveAction(keyboard_action,   Vector3f( 0.f, -1.f,  0.f)); break;
            
        // Rotate
        case KeyboardAction::YawLeft:       StartRotateAction(keyboard_action, Vector3f( 0.f, -1.f,  0.f) * rotation_axis_sign); break;
        case KeyboardAction::YawRight:      StartRotateAction(keyboard_action, Vector3f( 0.f,  1.f,  0.f) * rotation_axis_sign); break;
        case KeyboardAction::RollLeft:      StartRotateAction(keyboard_action, Vector3f( 0.f,  0.f,  1.f) * rotation_axis_sign); break;
        case KeyboardAction::RollRight:     StartRotateAction(keyboard_action, Vector3f( 0.f,  0.f, -1.f) * rotation_axis_sign); break;
        case KeyboardAction::PitchUp:       StartRotateAction(keyboard_action, Vector3f(-1.f,  0.f,  0.f) * rotation_axis_sign); break;
        case KeyboardAction::PitchDown:     StartRotateAction(keyboard_action, Vector3f( 1.f,  0.f,  0.f) * rotation_axis_sign); break;
            
        // Zoom
        case KeyboardAction::ZoomIn:        StartZoomAction(keyboard_action, 0.9f); break;
        case KeyboardAction::ZoomOut:       StartZoomAction(keyboard_action, 1.1f); break;
        
        // Other
        case KeyboardAction::Reset:         ResetOrientaion(); break;
        case KeyboardAction::ChangePivot:   SetPivot(m_pivot == Pivot::Aim ? Pivot::Eye : Pivot::Aim); break;
            
        default: return;
    }
}

void ActionCamera::OnKeyReleased(KeyboardAction keyboard_action)
{
    StopKeyboardAction(keyboard_action, m_keyboard_action_duration_sec);
}

Vector3f ActionCamera::GetNormalizedSphereProjection(const Point2i& mouse_screen_pos, bool is_primary) const
{
    const Point2f& screen_size = m_p_view_camera ? m_p_view_camera->GetScreenSize() : m_screen_size;
    const Point2f screen_center(screen_size.x() / 2.f, screen_size.y() / 2.f);
    Point2f screen_vector = static_cast<Point2f>(mouse_screen_pos) - screen_center;

    const float screen_radius = screen_vector.length();
    const float sphere_radius = GetRadiusInPixels(screen_size);

    // Primary screen point is used to determine if rotation is done inside sphere (around X and Y axes) or outside (around Z axis)
    // For secondary screen point the primary result is used
    const bool inside_sphere = ( is_primary && screen_radius <= sphere_radius) ||
                               (!is_primary && std::fabs(m_mouse_pressed_on_sphere[2]) > 0.000001f);

    // Reflect coordinates for natural camera movement
    const Point2f mirror_multipliers = m_p_view_camera
                                     ? Point2f(inside_sphere ? 1.f : -1.f, -1.f ) *
                                       unitSign(cml::dot(GetLookDirection(m_mouse_pressed_orientation), m_p_view_camera->GetLookDirection()))
                                     : Point2f(-1.f, 1.f);
    screen_vector.setX(screen_vector.x() * mirror_multipliers.x());
    screen_vector.setY(screen_vector.y() * mirror_multipliers.y());

    // Handle rotation between 90 and 180 degrees when mouse overruns one sphere radius
    float z_sign = 1.f;
    if (!is_primary && inside_sphere && screen_radius > sphere_radius)
    {
        const uint32_t radius_mult = static_cast<uint32_t>(std::floorf(screen_radius / sphere_radius));
        if (radius_mult < 2)
        {
            const float vector_radius = sphere_radius * (radius_mult + 1) - screen_radius;
            screen_vector = screen_vector.normalize() * vector_radius;
            z_sign = std::powf(-1.f, static_cast<float>(radius_mult));
        }
        else
        {
            screen_vector = Point2f(0.f, 0.f);
            z_sign = -1.f;
        }
    }

    return cml::normalize(Vector3f(screen_vector, inside_sphere ? z_sign * std::sqrtf(square(sphere_radius) - screen_vector.length_squared()) : 0.f));
}

void ActionCamera::ApplyLookDirection(const Vector3f& look_dir, const Orientation& base_orientation)
{
    switch (m_pivot)
    {
    case Pivot::Aim: m_current_orientation.eye = base_orientation.aim - look_dir; break;
    case Pivot::Eye: m_current_orientation.aim = base_orientation.eye + look_dir; break;
    }
}

void ActionCamera::Rotate(const Vector3f& view_axis, float angle_rad, const Orientation& base_orientation)
{
    Matrix44f view_rotation_matrix = { };
    cml::matrix_rotation_axis_angle(view_rotation_matrix, view_axis, angle_rad);
    
    const Vector4f look_in_view = m_p_view_camera
                                ? m_p_view_camera->TransformWorldToView(Vector4f(GetLookDirection(base_orientation), 1.f))
                                : Vector4f(0.f, 0.f, GetAimDistance(base_orientation), 1.f);
    
    const Vector3f look_dir     = m_p_view_camera
                                ? m_p_view_camera->TransformViewToWorld(view_rotation_matrix * look_in_view).subvector(3)
                                : TransformViewToWorld(view_rotation_matrix * look_in_view, base_orientation).subvector(3);
    
    const Vector4f up_in_view   = m_p_view_camera
                                ? m_p_view_camera->TransformWorldToView(Vector4f(base_orientation.up, 1.f))
                                : Vector4f(0.f, base_orientation.up.length(), 0.f, 1.f);
    
    m_current_orientation.up    = m_p_view_camera
                                ? m_p_view_camera->TransformViewToWorld(view_rotation_matrix * up_in_view).subvector(3)
                                : TransformViewToWorld(view_rotation_matrix * up_in_view, base_orientation).subvector(3);
    
    ApplyLookDirection(look_dir, base_orientation);
}

void ActionCamera::Move(const Vector3f& move_vector)
{
    m_current_orientation.aim += move_vector;
    m_current_orientation.eye += move_vector;
}

void ActionCamera::Zoom(float zoom_factor)
{
    const Vector3f look_dir   = GetLookDirection(m_current_orientation);
    const float zoom_distance = std::min(std::max(look_dir.length() * zoom_factor, m_zoom_distance_range.first), m_zoom_distance_range.second);
    ApplyLookDirection(cml::normalize(look_dir) * zoom_distance);
}

void ActionCamera::StartRotateAction(KeyboardAction rotate_action, const Vector3f& rotation_axis_in_view, double duration_sec)
{
    if (StartKeyboardAction(rotate_action, duration_sec))
        return;
    
    const float angle_rad_per_second = cml::rad(m_rotate_angle_per_second);
    m_animations.push_back(
        std::make_shared<TimeAnimation>([this, angle_rad_per_second, rotation_axis_in_view](double elapsed_seconds, double delta_seconds)
            {
                Rotate(rotation_axis_in_view, static_cast<float>(angle_rad_per_second * delta_seconds * GetAccelerationFactor(elapsed_seconds)));
                return true;
            },
            duration_sec));
    
    const auto emplace_result = m_keyboard_action_animations.emplace(rotate_action, m_animations.back());
    assert(emplace_result.second);
    if (!emplace_result.second) // fixes unused variable warning in release build
        return;
}

void ActionCamera::StartMoveAction(KeyboardAction move_action, const Vector3f& move_direction_in_view, double duration_sec)
{
    if (StartKeyboardAction(move_action, duration_sec))
        return;
    
    m_animations.push_back(
        std::make_shared<TimeAnimation>([this, move_direction_in_view](double elapsed_seconds, double delta_seconds)
            {
                const Vector3f move_per_second = TransformViewToWorld(move_direction_in_view).normalize() * m_move_distance_per_second;
                Move(move_per_second * delta_seconds * GetAccelerationFactor(elapsed_seconds));
                return true;
            },
            duration_sec));
    
    const auto emplace_result = m_keyboard_action_animations.emplace(move_action, m_animations.back());
    assert(emplace_result.second);
    if (!emplace_result.second) // fixes unused variable warning in release build
        return;
}

void ActionCamera::StartZoomAction(KeyboardAction zoom_action, float zoom_factor_per_second, double duration_sec)
{
    if (StartKeyboardAction(zoom_action, duration_sec))
        return;
    
    m_animations.push_back(
        std::make_shared<TimeAnimation>([this, zoom_factor_per_second](double elapsed_seconds, double delta_seconds)
            {
                Zoom(1.f - static_cast<float>((1.f - zoom_factor_per_second) * delta_seconds * GetAccelerationFactor(elapsed_seconds)));
                return true;
            },
            duration_sec));
    
    const auto emplace_result = m_keyboard_action_animations.emplace(zoom_action, m_animations.back());
    assert(emplace_result.second);
    if (!emplace_result.second) // fixes unused variable warning in release build
        return;
}

bool ActionCamera::StartKeyboardAction(KeyboardAction keyboard_action, double duration_sec)
{
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
    switch (mouse_action)
    {
    case MouseAction::Rotate:   return "rotate";
    case MouseAction::Zoom:     return "zoom";
    case MouseAction::Move:     return "move";
    case MouseAction::None:     return "none";
    default: assert(0);         return "";
    }
}

std::string ActionCamera::GetActionName(KeyboardAction keyboard_action)
{
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
    default: assert(0);                 return "";
    }
}