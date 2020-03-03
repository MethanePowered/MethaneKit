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

FILE: Methane/Graphics/ArcBallCamera.cpp
Arc-ball camera rotation with mouse handling.

******************************************************************************/

#include <Methane/Graphics/ArcBallCamera.h>
#include <Methane/Instrumentation.h>

#include <cml/mathlib/mathlib.h>

#include <cmath>
#include <cassert>

using namespace Methane::Data;

namespace Methane::Graphics
{

static inline float Square(float x)     { return x * x; }
static inline float UnitSign(float x)   { return x / std::fabs(x); }

ArcBallCamera::ArcBallCamera(Pivot pivot, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_p_view_camera(nullptr)
    , m_pivot(pivot)
{
    ITT_FUNCTION_TASK();
}

ArcBallCamera::ArcBallCamera(const Camera& view_camera, Pivot pivot, cml::AxisOrientation axis_orientation)
    : Camera(axis_orientation)
    , m_p_view_camera(&view_camera)
    , m_pivot(pivot)
{
    ITT_FUNCTION_TASK();
}

void ArcBallCamera::OnMousePressed(const Point2i& mouse_screen_pos)
{
    ITT_FUNCTION_TASK();
    
    m_mouse_pressed_orientation = m_current_orientation;
    m_mouse_pressed_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, true);
}

void ArcBallCamera::OnMouseDragged(const Point2i& mouse_screen_pos)
{
    ITT_FUNCTION_TASK();

    const Vector3f mouse_current_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, false);
    const Vector3f vectors_cross = cml::cross(m_mouse_pressed_on_sphere, mouse_current_on_sphere);
    const Vector3f rotation_axis = vectors_cross.normalize();
    const float    rotation_angle = std::atan2(vectors_cross.length(), cml::dot(m_mouse_pressed_on_sphere, mouse_current_on_sphere));

    Rotate(rotation_axis, rotation_angle, m_mouse_pressed_orientation);

    // NOTE: fixes rotation axis flip at angles approaching to 180 degrees
    if (std::abs(rotation_angle) > cml::rad(90.f))
    {
        m_mouse_pressed_orientation = m_current_orientation;
        m_mouse_pressed_on_sphere = mouse_current_on_sphere;
    }
}

Vector3f ArcBallCamera::GetNormalizedSphereProjection(const Point2i& mouse_screen_pos, bool is_primary) const
{
    ITT_FUNCTION_TASK();
    const Point2f& screen_size = m_p_view_camera ? m_p_view_camera->GetScreenSize() : m_screen_size;
    const Point2f screen_center(screen_size.GetX() / 2.f, screen_size.GetY() / 2.f);
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
                                           UnitSign(cml::dot(GetLookDirection(m_mouse_pressed_orientation), m_p_view_camera->GetLookDirection()))
                                     : Point2f(-1.f, 1.f);
    screen_vector.SetX(screen_vector.GetX() * mirror_multipliers.GetX());
    screen_vector.SetY(screen_vector.GetY() * mirror_multipliers.GetY());

    // Handle rotation between 90 and 180 degrees when mouse overruns one sphere radius
    float z_sign = 1.f;
    if (!is_primary && inside_sphere && screen_radius > sphere_radius)
    {
        const uint32_t radius_mult = static_cast<uint32_t>(std::floor(screen_radius / sphere_radius));
        if (radius_mult < 2)
        {
            const float vector_radius = sphere_radius * (radius_mult + 1) - screen_radius;
            screen_vector = screen_vector.normalize() * vector_radius;
            z_sign = std::pow(-1.f, static_cast<float>(radius_mult));
        }
        else
        {
            screen_vector = Point2f(0.f, 0.f);
            z_sign = -1.f;
        }
    }

    return cml::normalize(Vector3f(screen_vector, inside_sphere ? z_sign * std::sqrt(Square(sphere_radius) - screen_vector.length_squared()) : 0.f));
}

void ArcBallCamera::ApplyLookDirection(const Vector3f& look_dir)
{
    ITT_FUNCTION_TASK();
    switch (m_pivot)
    {
    case Pivot::Aim: m_current_orientation.eye = m_current_orientation.aim - look_dir; break;
    case Pivot::Eye: m_current_orientation.aim = m_current_orientation.eye + look_dir; break;
    }
    PrintOrientation();
}

void ArcBallCamera::Rotate(const Vector3f& view_axis, float angle_rad, const Orientation& base_orientation)
{
    ITT_FUNCTION_TASK();
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

    ApplyLookDirection(look_dir);
}

} // namespace Methane::Graphics
