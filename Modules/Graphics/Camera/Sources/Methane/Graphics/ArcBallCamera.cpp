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

FILE: Methane/Graphics/ArcBallCamera.cpp
Arc-ball camera rotation with mouse handling.

******************************************************************************/

#include <Methane/Graphics/ArcBallCamera.h>
#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Point.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <cmath>
#include <numbers>

namespace Methane::Graphics
{

[[nodiscard]]
static inline float Square(float x)     { return x * x; }

[[nodiscard]]
static inline float UnitSign(float x)   { return x >= 0.F ? 1.F : -1.F; }

ArcBallCamera::ArcBallCamera(Pivot pivot) noexcept
    : Camera()
    , m_pivot(pivot)
{ }

ArcBallCamera::ArcBallCamera(const Camera& view_camera, Pivot pivot) noexcept
    : Camera()
    , m_view_camera_ptr(&view_camera)
    , m_pivot(pivot)
{ }

void ArcBallCamera::MousePress(const Point2I& mouse_screen_pos) noexcept
{
    META_FUNCTION_TASK();
    m_mouse_pressed_orientation = GetOrientation();
    m_mouse_pressed_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, true);
}

void ArcBallCamera::MouseDrag(const Point2I& mouse_screen_pos)
{
    META_FUNCTION_TASK();

    const SphereProjection mouse_current_on_sphere = GetNormalizedSphereProjection(mouse_screen_pos, false);
    hlslpp::float3 vectors_cross = hlslpp::cross(m_mouse_pressed_on_sphere.vector, mouse_current_on_sphere.vector);
    const float vectors_angle_sin = hlslpp::length(vectors_cross);
    if (vectors_angle_sin <= 1E-7F)
    {
        static const hlslpp::float3 s_z_axis(0.F, 0.F, 1.F);
        vectors_cross = m_mouse_pressed_on_sphere.inside
                      ? hlslpp::cross(m_mouse_pressed_on_sphere.vector, s_z_axis)
                      : s_z_axis; // mouse was pressed inside sphere => rotation around z
    }
    const hlslpp::float3 rotation_axis     = hlslpp::normalize(vectors_cross);
    const float    vectors_angle_cos = hlslpp::dot(m_mouse_pressed_on_sphere.vector, mouse_current_on_sphere.vector);
    const float    rotation_angle    = std::atan2(vectors_angle_sin, vectors_angle_cos);

    RotateInView(rotation_axis, rotation_angle, m_mouse_pressed_orientation);

    // NOTE: fixes rotation axis flip at angles approaching to 180 degrees
    if (std::abs(rotation_angle) > std::numbers::pi / 2.f)
    {
        m_mouse_pressed_orientation = GetOrientation();
        m_mouse_pressed_on_sphere = mouse_current_on_sphere;
    }
}

ArcBallCamera::SphereProjection ArcBallCamera::GetNormalizedSphereProjection(const Point2I& mouse_screen_pos, bool is_primary) const noexcept
{
    META_FUNCTION_TASK();
    const Data::FloatSize& screen_size = m_view_camera_ptr ? m_view_camera_ptr->GetScreenSize() : GetScreenSize();
    const Point2F screen_center(screen_size.GetWidth() / 2.F, screen_size.GetHeight() / 2.F);
    Point2F       screen_point = static_cast<Point2F>(mouse_screen_pos) - screen_center;

    const float screen_radius = screen_point.GetLength();
    const float sphere_radius = GetRadiusInPixels(screen_size);

    // Primary screen point is used to determine if rotation is done inside sphere (around X and Y axes) or outside (around Z axis)
    // For secondary screen point the primary result is used
    const bool inside_sphere = ( is_primary && screen_radius <= sphere_radius) ||
                               (!is_primary && m_mouse_pressed_on_sphere.inside);
    const float inside_sphere_sign = inside_sphere ? 1.F : -1.F;

    // Reflect coordinates for natural camera movement
    const Point2F mirror_multipliers = m_view_camera_ptr
                                     ? Point2F(inside_sphere_sign, -1.F ) * UnitSign(hlslpp::dot(GetLookDirection(m_mouse_pressed_orientation), m_view_camera_ptr->GetLookDirection()))
                                     : Point2F(-1.F, 1.F);
    screen_point.SetX(screen_point.GetX() * mirror_multipliers.GetX());
    screen_point.SetY(screen_point.GetY() * mirror_multipliers.GetY());

    // Handle rotation between 90 and 180 degrees when mouse overruns one sphere radius
    float z_sign = 1.F;
    if (!is_primary && inside_sphere && screen_radius > sphere_radius)
    {
        if (const auto radius_mult = static_cast<uint32_t>(std::floor(screen_radius / sphere_radius));
            radius_mult < 2)
        {
            const float vector_radius = sphere_radius * static_cast<float>(radius_mult + 1) - screen_radius;
            screen_point = screen_point.Normalize() * vector_radius;
            z_sign       = std::pow(-1.F, static_cast<float>(radius_mult));
        }
        else
        {
            screen_point = Point2F(0.F, 0.F);
            z_sign       = -1.F;
        }
    }

    const float sphere_z = inside_sphere ? z_sign * std::sqrt(Square(sphere_radius) - screen_point.GetLengthSquared()) : 0.F;
    return SphereProjection{
        hlslpp::normalize(hlslpp::float3(screen_point.GetX(), screen_point.GetY(), sphere_z)),
        inside_sphere
    };
}

void ArcBallCamera::ApplyLookDirection(const hlslpp::float3& look_dir)
{
    META_FUNCTION_TASK();
    switch (m_pivot)
    {
    case Pivot::Aim: SetOrientationEye(GetOrientation().aim - look_dir); break;
    case Pivot::Eye: SetOrientationAim(GetOrientation().eye + look_dir); break;
    default:         META_UNEXPECTED(m_pivot);
    }
    Camera::LogOrientation();
}

void ArcBallCamera::RotateInView(const hlslpp::float3& view_axis, float angle_rad, const Orientation& base_orientation)
{
    META_FUNCTION_TASK();
    const hlslpp::float4x4 view_rotation_matrix = hlslpp::float4x4::rotation_axis(view_axis, -angle_rad);
    const Camera* view_camera_ptr = GetExternalViewCamera();

    const hlslpp::float4 look_in_view = view_camera_ptr
                                      ? view_camera_ptr->TransformWorldToView(hlslpp::float4(GetLookDirection(base_orientation), 1.F))
                                      : hlslpp::float4(0.F, 0.F, GetAimDistance(base_orientation), 1.F);

    const hlslpp::float3 look_dir     = view_camera_ptr
                                      ? view_camera_ptr->TransformViewToWorld(hlslpp::mul(look_in_view, view_rotation_matrix)).xyz
                                      : TransformViewToWorld(hlslpp::mul(look_in_view, view_rotation_matrix), base_orientation).xyz;

    const hlslpp::float4 up_in_view   = view_camera_ptr
                                      ? view_camera_ptr->TransformWorldToView(hlslpp::float4(base_orientation.up, 1.F))
                                      : hlslpp::float4(0.F, hlslpp::length(base_orientation.up), 0.F, 1.F);

    SetOrientationUp(view_camera_ptr
        ? view_camera_ptr->TransformViewToWorld(hlslpp::mul(up_in_view, view_rotation_matrix)).xyz
        : TransformViewToWorld(hlslpp::mul(up_in_view, view_rotation_matrix), base_orientation).xyz
    );

    ApplyLookDirection(look_dir);
}

} // namespace Methane::Graphics
