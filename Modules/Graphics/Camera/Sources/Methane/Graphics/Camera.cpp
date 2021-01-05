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

FILE: Methane/Graphics/Camera.cpp
Camera helper implementation allowing to generate view and projection matrices.

******************************************************************************/

#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <cml/mathlib/mathlib.h>
#include <hlsl++.h>

namespace Methane::Graphics
{

Camera::Camera(bool is_left_handed_axes) noexcept
    : m_is_left_handed_axes(is_left_handed_axes)
{
    META_FUNCTION_TASK();
    ResetOrientation();
}

void Camera::Rotate(const Vector3f& axis, float angle_deg) noexcept
{
    META_FUNCTION_TASK();
    const Matrix33f rotation_matrix = hlslpp::float3x3_rotate_axis(axis, cml::rad(angle_deg));
    const Vector3f new_look_dir = hlslpp::mul(GetLookDirection(), rotation_matrix);
    SetOrientationEye(GetOrientation().aim - new_look_dir);
}

Matrix44f Camera::CreateViewMatrix(const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::float4x4_look_at(orientation.eye, orientation.aim, orientation.up, m_is_left_handed_axes);
}

Matrix44f Camera::CreateProjMatrix() const
{
    META_FUNCTION_TASK();
    switch (m_projection)
    {
    case Projection::Perspective:
        return hlslpp::float4x4_perspective_fovy(GetFovAngleY(), m_aspect_ratio, m_parameters.near_depth, m_parameters.far_depth, true, m_is_left_handed_axes);

    case Projection::Orthogonal:
        return hlslpp::float4x4_orthographic(m_screen_size.width, m_screen_size.height, m_parameters.near_depth, m_parameters.far_depth, true, m_is_left_handed_axes);

    default:
        META_UNEXPECTED_ENUM_ARG(m_projection);
    }
}

const Matrix44f& Camera::GetViewMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_view_matrix_dirty)
        return m_current_view_matrix;

    m_current_view_matrix = CreateViewMatrix(m_current_orientation);
    m_is_current_view_matrix_dirty = false;
    return m_current_view_matrix;
}

const Matrix44f& Camera::GetProjMatrix() const
{
    META_FUNCTION_TASK();
    if (!m_is_current_proj_matrix_dirty)
        return m_current_proj_matrix;

    m_current_proj_matrix = CreateProjMatrix();
    m_is_current_proj_matrix_dirty = false;
    return m_current_proj_matrix;
}

const Matrix44f& Camera::GetViewProjMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_view_matrix_dirty && !m_is_current_proj_matrix_dirty)
        return m_current_view_proj_matrix;

    m_current_view_proj_matrix = hlslpp::mul(GetViewMatrix(), GetProjMatrix());
    return m_current_view_proj_matrix;
}

Vector2f Camera::TransformScreenToProj(const Data::Point2i& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return { 2.F * static_cast<float>(screen_pos.GetX()) / m_screen_size.width  - 1.F,
           -(2.F * static_cast<float>(screen_pos.GetY()) / m_screen_size.height - 1.F) };
}

Vector3f Camera::TransformScreenToView(const Data::Point2i& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::mul(hlslpp::inverse(GetProjMatrix()), Vector4f(TransformScreenToProj(screen_pos), 0.F, 1.F)).xyz;
}

Vector3f Camera::TransformScreenToWorld(const Data::Point2i& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return TransformViewToWorld(TransformScreenToView(screen_pos));
}

Vector4f Camera::TransformWorldToView(const Vector4f& world_pos, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::mul(hlslpp::inverse(CreateViewMatrix(orientation)), world_pos);
}

Vector4f Camera::TransformViewToWorld(const Vector4f& view_pos, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::mul(CreateViewMatrix(orientation), view_pos);
}

float Camera::GetFovAngleY() const noexcept
{
    META_FUNCTION_TASK();
    float fov_angle_y = m_parameters.fov_deg * cml::constants<float>::pi() / 180.0F;
    if (m_aspect_ratio != 0.F && m_aspect_ratio < 1.0F)
    {
        fov_angle_y /= m_aspect_ratio;
    }
    return fov_angle_y;
}

std::string Camera::GetOrientationString() const
{
    return fmt::format("Camera orientation:\n  - eye: {}\n  - aim: {}\n  - up:  {}",
                       m_current_orientation.eye, m_current_orientation.aim, m_current_orientation.up);
}

} // namespace Methane::Graphics
