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

namespace Methane
{
static inline float square(float x) { return x * x; }
}

ArcBallCamera::ArcBallCamera(Pivot pivot, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_pivot(pivot)
{
}

void ArcBallCamera::OnMousePressed(const Point2i& mouse_screen_pos)
{
    m_mouse_pressed_orientation = m_current_orientation;
    m_mouse_pressed_on_sphere   = GetNormalizedSphereProjection(mouse_screen_pos, true);
}

void ArcBallCamera::OnMouseDragged(const Point2i& mouse_screen_pos)
{
    const Vector3f mouse_current_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, false);
    const Vector3f rotation_axis  = cml::cross(m_mouse_pressed_on_sphere, mouse_current_on_sphere);
    const float    rotation_angle = std::acosf(cml::dot(m_mouse_pressed_on_sphere, mouse_current_on_sphere));

    Matrix44f view_rotation_matrix = { };
    cml::matrix_rotation_axis_angle(view_rotation_matrix, rotation_axis, rotation_angle);

    const Vector4f look_dir_in_view(0.f, 0.f, GetAimDistance(m_mouse_pressed_orientation), 1.f);
    const Vector3f look_dir = TransformViewToWorld(view_rotation_matrix * look_dir_in_view, m_mouse_pressed_orientation).subvector(3);

    const Vector4f orientation_up_in_view = TransformWorldToView(Vector4f(m_mouse_pressed_orientation.up, 1.f), m_mouse_pressed_orientation);
    m_current_orientation.up = TransformViewToWorld(view_rotation_matrix * orientation_up_in_view, m_mouse_pressed_orientation).subvector(3);

    switch(m_pivot)
    {
    case Pivot::Aim: m_current_orientation.eye = m_mouse_pressed_orientation.aim - look_dir; break;
    case Pivot::Eye: m_current_orientation.aim = m_mouse_pressed_orientation.eye + look_dir; break;
    }
}

Vector3f ArcBallCamera::GetNormalizedSphereProjection(const Point2i& mouse_screen_pos, bool is_primary) const
{
    const Point2f screen_center(m_width / 2.f, m_height / 2.f);
    Point2f screen_vector = static_cast<Point2f>(mouse_screen_pos) - screen_center;
    const float   screen_radius = screen_vector.length();
    const float   sphere_radius = std::min(screen_center.x(), screen_center.y()) * m_radius_ratio;
    const bool    inside_sphere =  (is_primary && screen_radius < sphere_radius) ||
                                  (!is_primary && std::fabs(m_mouse_pressed_on_sphere[2]) > std::numeric_limits<float>::lowest());

    float z_sign = 1.f;
    if (!is_primary && inside_sphere && screen_radius >= sphere_radius)
    {
        screen_vector.normalize();
        screen_vector *= sphere_radius * 2.f - screen_radius;
        z_sign = -1.f;
    }

    return cml::normalize(Vector3f(screen_vector, inside_sphere ? z_sign * std::sqrtf(square(sphere_radius) - screen_vector.length_squared()) : 0.f));
}

const Vector3f& ArcBallCamera::GetPivotPoint(const Orientation& orientation) const
{
    switch (m_pivot)
    {
    case Pivot::Aim: return orientation.aim;
    case Pivot::Eye: return orientation.eye;
    }
}
