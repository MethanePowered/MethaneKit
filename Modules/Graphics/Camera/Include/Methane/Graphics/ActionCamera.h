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

FILE: Methane/Graphics/ActionCamera.h
Interactive action-camera for rotating, moving and zooming with mouse and keyboard.

******************************************************************************/

#pragma once

#include "ArcBallCamera.h"

#include <Methane/Data/AnimationsPool.h>

#include <map>
#include <chrono>

namespace Methane::Graphics
{

class ActionCamera : public ArcBallCamera
{
public:
    enum class MouseAction : uint32_t
    {
        None = 0,

        Rotate,
        Move,
        Zoom,

        // Keep at end
        Count
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

    ActionCamera(Data::AnimationsPool& animations, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation) noexcept;
    ActionCamera(const Camera& view_camera, Data::AnimationsPool& animations, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation) noexcept;

    // Parameters
    uint32_t GetZoomStepsCount() const noexcept                             { return m_zoom_steps_count; }
    void     SetZoomStepsCount(uint32_t steps_count) noexcept               { m_zoom_steps_count = steps_count;}

    const DistanceRange& GetZoomDistanceRange() const noexcept              { return m_zoom_distance_range; }
    void SetZoomDistanceRange(const DistanceRange& distance_range) noexcept { m_zoom_distance_range = distance_range; }
    
    float GetRotateAnglePerSecond() const noexcept                          { return m_rotate_angle_per_second; }
    void  SetRotateAnglePerSecond(float rotate_angle_per_second) noexcept   { m_rotate_angle_per_second = rotate_angle_per_second; }

    float GetMoveDistancePerSecond() const noexcept                         { return m_move_distance_per_second; }
    void  SetMoveDistancePerSecond(float distance_per_second) noexcept      { m_move_distance_per_second = distance_per_second; }

    double GetKeyboardActionDurationSec() const noexcept                    { return m_keyboard_action_duration_sec; }
    void   SetKeyboardActionDurationSec(double min_duration_sec) noexcept   { m_keyboard_action_duration_sec = min_duration_sec; }

    // Mouse action handlers
    void OnMousePressed(const Data::Point2i& mouse_screen_pos, MouseAction mouse_action) noexcept;
    void OnMouseDragged(const Data::Point2i& mouse_screen_pos);
    void OnMouseReleased(const Data::Point2i&) noexcept;
    void OnMouseScrolled(float scroll_delta);

    // Keyboard action handlers
    void OnKeyPressed(KeyboardAction keyboard_action);
    void OnKeyReleased(KeyboardAction keyboard_action);
    void DoKeyboardAction(KeyboardAction keyboard_action) noexcept;

    static std::string GetActionName(MouseAction mouse_action);
    static std::string GetActionName(KeyboardAction keyboard_action);

protected:
    using KeyboardActionAnimations  = std::map<KeyboardAction, WeakPtr<Data::Animation>>;

    void Move(const Vector3f& move_vector) noexcept;
    void Zoom(float zoom_factor) noexcept;

    inline double GetAccelerationFactor(double elapsed_seconds) const noexcept { return std::max(1.0, elapsed_seconds / m_keyboard_action_duration_sec); }
    
    void StartRotateAction(KeyboardAction rotate_action, const Vector3f& rotation_axis,
                           double duration_sec = std::numeric_limits<double>::max());
    
    void StartMoveAction(KeyboardAction move_action, const Vector3f& move_direction_in_view,
                         double duration_sec = std::numeric_limits<double>::max());
    
    void StartZoomAction(KeyboardAction zoom_action, float zoom_factor_per_second,
                         double duration_sec = std::numeric_limits<double>::max());
    
    bool StartKeyboardAction(KeyboardAction keyboard_action, double duration_sec);
    bool StopKeyboardAction(KeyboardAction keyboard_action, double duration_sec);

private:
    Data::AnimationsPool&    m_animations;
    uint32_t                 m_zoom_steps_count             = 3;
    DistanceRange            m_zoom_distance_range          = DistanceRange(1.F, 1000.F);
    float                    m_move_distance_per_second     = 5.F;
    float                    m_rotate_angle_per_second      = 15.F;
    double                   m_keyboard_action_duration_sec = 0.3;
    MouseAction              m_mouse_action                 = MouseAction::None;
    Vector3f                 m_mouse_pressed_in_world       { };
    KeyboardActionAnimations m_keyboard_action_animations;
};

} // namespace Methane::Graphics
