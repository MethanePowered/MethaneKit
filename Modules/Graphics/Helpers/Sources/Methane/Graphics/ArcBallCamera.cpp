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

FILE: Methane/Graphics/ArcBallCamera.cpp
Arc-ball camera implementation.

******************************************************************************/

#include <Methane/Graphics/ArcBallCamera.h>

#include <cml/mathlib/mathlib.h>

#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
    #include <Methane/Platform/Utils.h>
    using namespace Methane::Platform;
#endif

using namespace Methane::Data;
using namespace Methane::Graphics;

Quaternionf QuaternionFromUnitSphere(const Vector3f& from, const Vector3f& to)
{
    return {
        from[1] * to[2] - from[2] * to[1],
        from[2] * to[0] - from[0] * to[2],
        from[0] * to[1] - from[1] * to[0],
        from[0] * to[0] + from[1] * to[1] + from[2] * to[2]
    };
}

ArcBallCamera::ArcBallCamera(Pivot pivot, cml::AxisOrientation axis_orientation)
    : ArcBallCamera(*this, pivot, axis_orientation)
{
}

ArcBallCamera::ArcBallCamera(Camera& view_camera, Pivot pivot, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_view_camera(view_camera)
    , m_pivot(pivot)
{
}

void ArcBallCamera::OnMousePressed(const Point2i& mouse_screen_pos)
{
    const Vector3f mouse_pressed_in_view = GetViewFromScreenPos(mouse_screen_pos).subvector(3);
    m_mouse_pressed_orientation          = m_current_orientation;
    m_mouse_pressed_on_sphere            = GetSphereProjection(mouse_pressed_in_view);

#ifdef DEBUG_OUTPUT
    PrintToDebugOutput("Mouse pressed: " + std::string(mouse_screen_pos) +
                       ", in view: "    + VectorToString(mouse_pressed_in_view));
#endif
}

void ArcBallCamera::OnMouseDragged(const Point2i& mouse_screen_pos)
{
    const Vector3f mouse_current_in_view   = GetViewFromScreenPos(mouse_screen_pos).subvector(3);
    const Vector3f mouse_current_on_sphere = GetSphereProjection(mouse_current_in_view);
    Quaternionf    mouse_rotation_quat     = QuaternionFromUnitSphere(m_mouse_pressed_on_sphere, mouse_current_on_sphere).conjugate();

    Matrix44f rotation_matrix = { };
    cml::matrix_rotation_quaternion(rotation_matrix, mouse_rotation_quat);

    Vector3f  look_dir = m_mouse_pressed_orientation.aim - m_mouse_pressed_orientation.eye;
    look_dir = (rotation_matrix * Vector4f(look_dir, 1.f)).subvector(3);

    m_current_orientation.up = (rotation_matrix * Vector4f(m_mouse_pressed_orientation.up, 1.f)).subvector(3);

    switch(m_pivot)
    {
    case Pivot::Aim: m_current_orientation.eye = m_mouse_pressed_orientation.aim - look_dir; break;
    case Pivot::Eye: m_current_orientation.aim = m_mouse_pressed_orientation.eye + look_dir; break;
    }

#ifdef DEBUG_OUTPUT
    PrintToDebugOutput("Mouse dragged: " + std::string(mouse_screen_pos) +
        ", in view: " + VectorToString(mouse_current_in_view));
#endif
}

Vector3f ArcBallCamera::GetSphereProjection(const Vector3f& view_pos)
{
    const Vector3f view_pivot_point = (GetViewMatrix(m_mouse_pressed_orientation) * Vector4f(GetPivotPoint(m_mouse_pressed_orientation), 1.f)).subvector(3);

    // (m - C) / R
    Vector3f sphere_pos = (view_pos - view_pivot_point) / m_radius;
    sphere_pos[2] = 0.f;

    const float mag = cml::dot(sphere_pos, sphere_pos);
    if (mag > 1.0)
    {
        // Since we are outside of the sphere, map to the visible boundary of the sphere.
        sphere_pos /= sqrtf(mag);
    }
    else
    {
        // We are not at the edge of the sphere, we are inside of it.
        // Essentially, we are normalizing the vector by adding the missing z component.
        sphere_pos[2] = sqrtf(1.f - mag);
    }

    return sphere_pos;
}

const Vector3f& ArcBallCamera::GetPivotPoint(const Orientation& orientation) const
{
    switch (m_pivot)
    {
    case Pivot::Aim: return orientation.aim;
    case Pivot::Eye: return orientation.eye;
    }
}