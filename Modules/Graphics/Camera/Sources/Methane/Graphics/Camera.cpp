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

#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Instrumentation.h>

#include <cml/mathlib/mathlib.h>
#include <sstream>

namespace Methane::Graphics
{

Camera::Camera(cml::AxisOrientation axis_orientation) noexcept
    : m_axis_orientation(axis_orientation)
{
    META_FUNCTION_TASK();
    ResetOrientation();
}

void Camera::Rotate(const Vector3f& axis, float angle_deg) noexcept
{
    META_FUNCTION_TASK();
    Matrix33f rotation_matrix{ };
    cml::matrix_rotation_axis_angle(rotation_matrix, axis, cml::rad(angle_deg));
    Vector3f new_look_dir = GetLookDirection() * rotation_matrix;
    SetOrientationEye(GetOrientation().aim - new_look_dir);
}

Matrix44f Camera::GetViewMatrix(const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    Matrix44f view_matrix{ };
    GetViewMatrix(view_matrix, orientation);
    return view_matrix;
}

void Camera::GetViewMatrix(Matrix44f& out_view, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    cml::matrix_look_at(out_view, orientation.eye, orientation.aim, orientation.up, m_axis_orientation);
}

void Camera::GetProjMatrix(Matrix44f& out_proj) const noexcept
{
    META_FUNCTION_TASK();
    switch (m_projection)
    {
    case Projection::Perspective:
        cml::matrix_perspective_yfov(out_proj, GetFovAngleY(), m_aspect_ratio, m_parameters.near_depth, m_parameters.far_depth, m_axis_orientation, cml::ZClip::z_clip_zero);
        break;
    case Projection::Orthogonal:
        cml::matrix_orthographic(out_proj, m_screen_size.width, m_screen_size.height, m_parameters.near_depth, m_parameters.far_depth, m_axis_orientation, cml::ZClip::z_clip_zero);
        break;
    }
}

const Matrix44f& Camera::GetViewMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_view_matrix_dirty)
        return m_current_view_matrix;

    GetViewMatrix(m_current_view_matrix);
    m_is_current_view_matrix_dirty = false;
    return m_current_view_matrix;
}

const Matrix44f& Camera::GetProjMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_proj_matrix_dirty)
        return m_current_proj_matrix;

    GetProjMatrix(m_current_proj_matrix);
    m_is_current_proj_matrix_dirty = false;
    return m_current_proj_matrix;
}

const Matrix44f& Camera::GetViewProjMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_view_matrix_dirty && !m_is_current_proj_matrix_dirty)
        return m_current_view_proj_matrix;

    m_current_view_proj_matrix = GetViewMatrix() * GetProjMatrix();
    return m_current_view_proj_matrix;
}

Vector2f Camera::TransformScreenToProj(const Data::Point2i& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return { 2.F * screen_pos.GetX() / m_screen_size.width  - 1.F,
           -(2.F * screen_pos.GetY() / m_screen_size.height - 1.F) };
}

Vector3f Camera::TransformScreenToView(const Data::Point2i& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return (cml::inverse(GetProjMatrix()) * Vector4f(TransformScreenToProj(screen_pos), 0.F, 1.F)).subvector(3);
}

Vector3f Camera::TransformScreenToWorld(const Data::Point2i& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return TransformViewToWorld(TransformScreenToView(screen_pos));
}

Vector4f Camera::TransformWorldToView(const Vector4f& world_pos, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return cml::inverse(GetViewMatrix(orientation)) * world_pos;
}

Vector4f Camera::TransformViewToWorld(const Vector4f& view_pos, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return GetViewMatrix(orientation) * view_pos;
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
    std::stringstream ss;
    ss << "Camera orientation:"
       << std::endl << "  - eye: " << VectorToString(m_current_orientation.eye)
       << std::endl << "  - aim: " << VectorToString(m_current_orientation.aim)
       << std::endl << "  - up:  " << VectorToString(m_current_orientation.up);
    return ss.str();
}

} // namespace Methane::Graphics
