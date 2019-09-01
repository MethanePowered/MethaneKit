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

FILE: Methane/Graphics/ActionCamera.h
Interactive action-camera for rotating, moving and zooming with mouse and keyboard.
Implemented based on arc-ball camera rotation model.

******************************************************************************/

#pragma once

#include "Camera.h"

#include <Methane/Data/AnimationsPool.h>

#include <map>
#include <chrono>

namespace Methane
{
namespace Graphics
{

class ActionCamera : public Camera
{
public:
    enum class Pivot : uint32_t
    {
        Aim = 0,
        Eye,
    };

    enum class MouseAction : uint32_t
    {
        None = 0,

        Rotate,
        Move,
        Zoom,

        Count // keep at end
    };

    enum class KeyboardAction : uint32_t
    {
        None = 0,

        // Move
        MoveLeft,
        MoveRight,
        MoveForward,
        MoveBack,
        MoveUp,
        MoveDown,

        // Rotate
        YawLeft,
        YawRight,
        RollLeft,
        RollRight,
        PitchUp,
        PitchDown,

        // Zoom
        ZoomIn,
        ZoomOut,
        
        // Other
        Reset,
        ChangePivot,

        // Keep at end
        Count
    };

    using DistanceRange = std::pair<float /*min_distance*/, float /*max_distance*/>;

    ActionCamera(Data::AnimationsPool& animations, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);
    ActionCamera(const Camera& view_camera, Data::AnimationsPool& animations, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);

    // Parameters
    Pivot GetPivot() const                                          { return m_pivot; }
    void  SetPivot(Pivot pivot)                                     { m_pivot = pivot; }

    float GetRadiusRatio() const                                    { return m_radius_ratio; }
    void  SetRadiusRatio(float radius_ratio)                        { m_radius_ratio = radius_ratio; }
    float GetRadiusInPixels() const noexcept                        { return GetRadiusInPixels(m_screen_size); }

    uint32_t GetZoomStepsCount() const                              { return m_zoom_steps_count; }
    void  SetZoomStepsCount(uint32_t steps_count)                   { m_zoom_steps_count = steps_count;}

    const DistanceRange& GetZoomDistanceRange() const               { return m_zoom_distance_range; }
    void SetZoomDistanceRange(const DistanceRange& distance_range)  { m_zoom_distance_range = distance_range; }
    
    float GetRotateAnglePerSecond() const                           { return m_rotate_angle_per_second; }
    void SetRotateAnglePerSecond(float rotate_angle_per_second)     { m_rotate_angle_per_second = rotate_angle_per_second; }

    float GetMoveDistancePerSecond() const                          { return m_move_distance_per_second; }
    void SetMoveDistancePerSecond(float distance_per_second)        { m_move_distance_per_second = distance_per_second; }

    double GetKeyboardActionDurationSec() const                     { return m_keyboard_action_duration_sec; }
    void SetKeyboardActionDurationSec(double min_duration_sec)      { m_keyboard_action_duration_sec = min_duration_sec; }

    // Mouse action handlers
    void OnMousePressed(const Data::Point2i& mouse_screen_pos, MouseAction mouse_action);
    void OnMouseDragged(const Data::Point2i& mouse_screen_pos);
    void OnMouseReleased(const Data::Point2i&);
    void OnMouseScrolled(float scroll_delta);

    // Keyboard action handlers
    void OnKeyPressed(KeyboardAction keyboard_action);
    void OnKeyReleased(KeyboardAction keyboard_action);
    void DoKeyboardAction(KeyboardAction keyboard_action);

    static std::string GetActionName(MouseAction mouse_action);
    static std::string GetActionName(KeyboardAction keyboard_action);

protected:
    using KeyboardActionAnimations  = std::map<KeyboardAction, Data::Animation::WeakPtr>;

    Vector3f GetNormalizedSphereProjection(const Data::Point2i& mouse_screen_pos, bool is_primary) const;

    inline float GetRadiusInPixels(const Data::Point2f& screen_size) const noexcept
    { return std::min(screen_size.x(), screen_size.y()) * m_radius_ratio / 2.f; }

    inline const Camera& GetViewCamera() const noexcept
    { return m_p_view_camera ? *m_p_view_camera : *this; }

    void ApplyLookDirection(const Vector3f& look_dir);
    void Rotate(const Vector3f& view_axis, float angle_rad, const Orientation& base_orientation);
    void Rotate(const Vector3f& view_axis, float angle_rad) { Rotate(view_axis, angle_rad, m_current_orientation ); }
    void Move(const Vector3f& move_vector);
    void Zoom(float zoom_factor);
    
    inline double GetAccelerationFactor(double elapsed_seconds) { return std::max(1.0, elapsed_seconds / m_keyboard_action_duration_sec); }
    
    void StartRotateAction(KeyboardAction rotate_action, const Vector3f& rotation_axis,
                           double duration_sec = std::numeric_limits<double>::max());
    
    void StartMoveAction(KeyboardAction move_action, const Vector3f& move_direction_in_view,
                         double duration_sec = std::numeric_limits<double>::max());
    
    void StartZoomAction(KeyboardAction zoom_action, float zoom_factor_per_second,
                         double duration_sec = std::numeric_limits<double>::max());
    
    bool StartKeyboardAction(KeyboardAction keyboard_action, double duration_sec);
    bool StopKeyboardAction(KeyboardAction keyboard_action, double duration_sec);

    Data::AnimationsPool&    m_animations;
    const Camera*            m_p_view_camera;
    Pivot                    m_pivot;
    float                    m_radius_ratio                 = 0.9f;
    uint32_t                 m_zoom_steps_count             = 3;
    DistanceRange            m_zoom_distance_range          = DistanceRange(1.f, 1000.f);
    float                    m_move_distance_per_second     = 5.f;
    float                    m_rotate_angle_per_second      = 15.f;
    double                   m_keyboard_action_duration_sec = 0.3;
    MouseAction              m_mouse_action                 = MouseAction::None;
    Vector3f                 m_mouse_pressed_on_sphere      = { };
    Vector3f                 m_mouse_pressed_in_world       = { };
    Orientation              m_mouse_pressed_orientation    = { };
    KeyboardActionAnimations m_keyboard_action_animations;
};

} // namespace Graphics
} // namespace Methane
