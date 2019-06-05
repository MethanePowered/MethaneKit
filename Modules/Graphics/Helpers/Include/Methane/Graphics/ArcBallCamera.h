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

FILE: Methane/Graphics/ArcBallCamera.h
Arc-ball camera implementation.

******************************************************************************/

#pragma once

#include "Camera.h"

#include <Methane/Data/Types.h>

#include <cml/quaternion.h>

namespace Methane
{
namespace Graphics
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
    ArcBallCamera(Camera& p_view_camera, Pivot pivot = Pivot::Aim, cml::AxisOrientation axis_orientation = g_axis_orientation);

    const Camera& GetViewCamera() const     { return m_view_camera; }
    Pivot         GetPivot() const          { return m_pivot; }

    float         GetRadius() const         { return m_radius; }
    void          SetRadius(float radius)   { m_radius = radius; }

    void OnMousePressed(const Data::Point2i& mouse_screen_pos);
    void OnMouseDragged(const Data::Point2i& mouse_screen_pos);

protected:
    const Vector3f& GetPivotPoint(const Orientation& orientation) const;
    Vector3f GetSphereProjection(const Vector3f& view_pos);

    const Camera&   m_view_camera;
    const Pivot     m_pivot;
    float           m_radius                    = 0.75;
    Vector3f        m_mouse_pressed_on_sphere   = { };
    Orientation     m_mouse_pressed_orientation = { };
};

} // namespace Graphics
} // namespace Methane
