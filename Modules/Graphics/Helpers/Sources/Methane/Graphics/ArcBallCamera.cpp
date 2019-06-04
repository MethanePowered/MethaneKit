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
    const Vector4f mouse_screen_vector(mouse_screen_pos.x(), mouse_screen_pos.y(), 0.f, 1.f);
    m_mouse_pressed_in_world  = (GetScreenToWorldMatrix() * mouse_screen_vector).subvector(3);
    m_mouse_pressed_on_sphere = GetSphereProjection(m_mouse_pressed_in_world);
    m_previous_quat = m_current_quat;
}

void ArcBallCamera::OnMouseDragged(const Point2i& mouse_screen_pos)
{
    const Vector4f mouse_screen_vector(mouse_screen_pos.x(), mouse_screen_pos.y(), 0.f, 1.f);
    const Vector3f mouse_current_in_world  = (GetScreenToWorldMatrix() * mouse_screen_vector).subvector(3);
    const Vector3f mouse_current_on_sphere = GetSphereProjection(mouse_current_in_world);
    m_current_quat = QuaternionFromUnitSphere(m_mouse_pressed_on_sphere, mouse_current_on_sphere); // *m_previous_quat;

    Quaternionf rotation_quat = m_current_quat;
    //rotation_quat.conjugate();

    Matrix44f rotation_matrix = { };
    cml::matrix_rotation_quaternion(rotation_matrix, rotation_quat);

    Vector3f  look_dir = m_current_orientation.aim - m_current_orientation.eye;
    Vector3f& up_dir   = m_current_orientation.up;

    look_dir = (rotation_matrix * Vector4f(look_dir, 1.f)).subvector(3);
    up_dir   = (rotation_matrix * Vector4f(up_dir, 1.f)).subvector(3);

    switch(m_pivot)
    {
    case Pivot::Aim: m_current_orientation.eye = m_current_orientation.aim - look_dir; break;
    case Pivot::Eye: m_current_orientation.aim = m_current_orientation.eye + look_dir; break;
    }
}

Matrix44f ArcBallCamera::GetScreenToWorldMatrix()
{
    Matrix44f view_matrix = { }, proj_matrix = { };
    m_view_camera.GetViewProjMatrices(view_matrix, proj_matrix);
    return cml::inverse(proj_matrix * view_matrix) * m_screen_to_proj_matrix;
}

Vector3f ArcBallCamera::GetSphereProjection(const Vector3f& world_pos)
{
    // (m - C) / R
    Vector3f sphere_pos = (world_pos - GetPivotPoint()) / m_radius;
    sphere_pos[2] = 0.f;

    float mag = cml::dot(sphere_pos, sphere_pos);
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

Vector3f& ArcBallCamera::GetPivotPoint()
{
    switch (m_pivot)
    {
    case Pivot::Aim: return m_current_orientation.aim;
    case Pivot::Eye: return m_current_orientation.eye;
    }
}