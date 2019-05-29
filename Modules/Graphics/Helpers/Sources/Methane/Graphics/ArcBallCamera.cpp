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

using namespace Methane::Graphics;

Quaternionf QuaternionFromUnitSphere(const Vector3f& from, const Vector3f& to)
{
    Quaternionf q;
    q[0] = from[1] * to[2] - from[2] * to[1];
    q[1] = from[2] * to[0] - from[0] * to[2];
    q[2] = from[0] * to[1] - from[1] * to[0];
    q[3] = from[0] * to[0] + from[1] * to[1] + from[2] * to[2];
    return q;
}

ArcBallCamera::ArcBallCamera(float radius, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_radius(radius)
{
}

void ArcBallCamera::OnMousePressed(const Vector2f& mouse_screen_pos)
{
    m_mouse_pressed_in_world  = (GetScreenToWorldMatrix() * Vector4f(mouse_screen_pos, 0.f, 1.f)).subvector(3);
    m_mouse_pressed_on_sphere = GetSphereProjection(m_mouse_pressed_in_world);
    m_previous_quat = m_final_quat;
}

void ArcBallCamera::OnMouseDragged(const Vector2f& mouse_screen_pos)
{
    m_mouse_current_in_world  = (GetScreenToWorldMatrix() * Vector4f(mouse_screen_pos, 0.f, 1.f)).subvector(3);
    m_mouse_current_on_sphere = GetSphereProjection(m_mouse_current_in_world);
    m_current_quat = QuaternionFromUnitSphere(m_mouse_pressed_on_sphere, m_mouse_current_on_sphere);
    m_final_quat = m_current_quat * m_previous_quat;
}

Matrix44f ArcBallCamera::GetScreenToWorldMatrix()
{
    Matrix44f screen_to_world;
    // TODO: calculate me!
    return screen_to_world;
}

Vector3f ArcBallCamera::GetSphereProjection(const Vector3f& world_pos)
{
    Vector3f sphere_pos;

    // (m - C) / R
    sphere_pos[0] = (world_pos[0] - m_current_orientation.eye[0]) / m_radius;
    sphere_pos[1] = (world_pos[1] - m_current_orientation.eye[1]) / m_radius;

    float mag = cml::dot(sphere_pos, sphere_pos);
    if (mag > 1.0)
    {
        // Since we are outside of the sphere, map to the visible boundary of the sphere.
        sphere_pos *= 1.f / sqrtf(mag);
        sphere_pos[3] = 0.f;
    }
    else
    {
        // We are not at the edge of the sphere, we are inside of it.
        // Essentially, we are normalizing the vector by adding the missing z component.
        sphere_pos[3] = sqrtf(1.f - mag);
    }

    return sphere_pos;
}