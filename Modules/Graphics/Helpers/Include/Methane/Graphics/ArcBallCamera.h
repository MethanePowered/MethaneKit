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

FILE: Methane/Graphics/ArcBallCamera.h
Arc-ball camera rotation with mouse handling.

******************************************************************************/

#pragma once

#include "Camera.h"

#include <map>
#include <chrono>

namespace Methane::Graphics
{

class ArcBallCamera : public Camera
{
public:
    enum class Pivot : uint32_t
    {
        Aim = 0,
        Eye,
    };

    ArcBallCamera(Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);
    ArcBallCamera(const Camera& view_camera, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);

    // Parameters
    Pivot GetPivot() const                      { return m_pivot; }
    void  SetPivot(Pivot pivot)                 { m_pivot = pivot; }

    float GetRadiusRatio() const                { return m_radius_ratio; }
    void  SetRadiusRatio(float radius_ratio)    { m_radius_ratio = radius_ratio; }
    float GetRadiusInPixels() const noexcept    { return GetRadiusInPixels(m_screen_size); }

    // Mouse action handlers
    void OnMousePressed(const Data::Point2i& mouse_screen_pos);
    void OnMouseDragged(const Data::Point2i& mouse_screen_pos);

protected:
    Vector3f GetNormalizedSphereProjection(const Data::Point2i& mouse_screen_pos, bool is_primary) const;

    inline float GetRadiusInPixels(const Data::Point2f& screen_size) const noexcept
    { return std::min(screen_size.GetX(), screen_size.GetY()) * m_radius_ratio / 2.f; }

    inline const Camera& GetViewCamera() const noexcept
    { return m_p_view_camera ? *m_p_view_camera : *this; }

    void ApplyLookDirection(const Vector3f& look_dir);
    void Rotate(const Vector3f& view_axis, float angle_rad, const Orientation& base_orientation);
    void Rotate(const Vector3f& view_axis, float angle_rad) { Rotate(view_axis, angle_rad, m_current_orientation ); }

    const Camera*            m_p_view_camera;
    Pivot                    m_pivot;
    float                    m_radius_ratio                 = 0.9f;
    Vector3f                 m_mouse_pressed_on_sphere      = { };
    Vector3f                 m_mouse_pressed_in_world       = { };
    Orientation              m_mouse_pressed_orientation    = { };
};

} // namespace Methane::Graphics
