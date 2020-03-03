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

FILE: Methane/Graphics/Camera.cpp
Camera helper implementation allowing to generate view and projection matrices.

******************************************************************************/

#include <Methane/Graphics/Camera.h>
#include <Methane/Instrumentation.h>

#include <cml/mathlib/mathlib.h>

//#define PRINT_CAMERA_ORIENTATION

#ifdef PRINT_CAMERA_ORIENTATION
#include <Methane/Platform/Utils.h>
#include <sstream>
#endif

namespace Methane::Graphics
{

Camera::Camera(cml::AxisOrientation axis_orientation)
    : m_axis_orientation(axis_orientation)
{
    ITT_FUNCTION_TASK();
    ResetOrientation();
}

void Camera::Resize(float width, float height) noexcept
{
    ITT_FUNCTION_TASK();
    m_screen_size = { width, height };
    m_aspect_ratio = width / height;
}

void Camera::RotateYaw(float deg) noexcept
{
    ITT_FUNCTION_TASK();
    Matrix33f rotation_matrix = { };
    cml::matrix_rotation_axis_angle(rotation_matrix, m_current_orientation.up, cml::rad(deg));
    m_current_orientation.eye = m_current_orientation.eye * rotation_matrix;
}

void Camera::RotatePitch(float deg) noexcept
{
    ITT_FUNCTION_TASK();
    Matrix33f rotation_matrix = { };
    auto right = cml::cross(m_current_orientation.eye, m_current_orientation.up).normalize();
    cml::matrix_rotation_axis_angle(rotation_matrix, right, cml::rad(deg));
    m_current_orientation.eye = m_current_orientation.eye * rotation_matrix;
}

void Camera::GetViewProjMatrices(Matrix44f& out_view, Matrix44f& out_proj) const noexcept
{
    ITT_FUNCTION_TASK();
    GetViewMatrix(out_view);
    GetProjMatrix(out_proj);
}

void Camera::GetViewMatrix(Matrix44f& out_view, const Orientation& orientation) const noexcept
{
    ITT_FUNCTION_TASK();
    cml::matrix_look_at(out_view, orientation.eye, orientation.aim, orientation.up, m_axis_orientation);
}

void Camera::GetProjMatrix(Matrix44f& out_proj) const noexcept
{
    ITT_FUNCTION_TASK();
    switch (m_projection)
    {
    case Projection::Perspective:
        cml::matrix_perspective_yfov(out_proj, GetFovAngleY(), m_aspect_ratio, m_parameters.near_depth, m_parameters.far_depth, m_axis_orientation, cml::ZClip::z_clip_zero);
        break;
    case Projection::Orthogonal:
        cml::matrix_orthographic(out_proj, m_screen_size.GetX(), m_screen_size.GetY(), m_parameters.near_depth, m_parameters.far_depth, m_axis_orientation, cml::ZClip::z_clip_zero);
        break;
    }
}

Matrix44f Camera::GetViewMatrix(const Orientation& orientation) const noexcept
{
    ITT_FUNCTION_TASK();
    Matrix44f view_matrix = { };
    GetViewMatrix(view_matrix, orientation);
    return view_matrix;
}

Matrix44f Camera::GetProjMatrix() const noexcept
{
    ITT_FUNCTION_TASK();
    Matrix44f proj_matrix = { };
    GetProjMatrix(proj_matrix);
    return proj_matrix;
}

Matrix44f Camera::GetViewProjMatrix() const noexcept
{
    ITT_FUNCTION_TASK();
    return GetViewMatrix() * GetProjMatrix();
}

Vector2f Camera::TransformScreenToProj(const Data::Point2i& screen_pos) const noexcept
{
    ITT_FUNCTION_TASK();
    return { 2.f * screen_pos.GetX() / m_screen_size.GetX() - 1.f,
           -(2.f * screen_pos.GetY() / m_screen_size.GetY() - 1.f) };
}

Vector3f Camera::TransformScreenToView(const Data::Point2i& screen_pos) const noexcept
{
    ITT_FUNCTION_TASK();
    return (cml::inverse(GetProjMatrix()) * Vector4f(TransformScreenToProj(screen_pos), 0.f, 1.f)).subvector(3);
}

Vector3f Camera::TransformScreenToWorld(const Data::Point2i& screen_pos) const noexcept
{
    ITT_FUNCTION_TASK();
    return TransformViewToWorld(TransformScreenToView(screen_pos));
}

Vector4f Camera::TransformWorldToView(const Vector4f& world_pos, const Orientation& orientation) const noexcept
{
    ITT_FUNCTION_TASK();
    return cml::inverse(GetViewMatrix(orientation)) * world_pos;
}

Vector4f Camera::TransformViewToWorld(const Vector4f& view_pos, const Orientation& orientation) const noexcept
{
    ITT_FUNCTION_TASK();
    return GetViewMatrix(orientation) * view_pos;
}

float Camera::GetFovAngleY() const noexcept
{
    ITT_FUNCTION_TASK();
    float fov_angle_y = m_parameters.fov_deg * cml::constants<float>::pi() / 180.0f;
    if (m_aspect_ratio != 0.f && m_aspect_ratio < 1.0f)
    {
        fov_angle_y /= m_aspect_ratio;
    }
    return fov_angle_y;
}

void Camera::PrintOrientation()
{
#ifdef PRINT_CAMERA_ORIENTATION
    std::stringstream ss;
    ss << std::endl << "Camera orientation:"
       << std::endl << "  - eye: " << VectorToString(m_current_orientation.eye)
       << std::endl << "  - aim: " << VectorToString(m_current_orientation.aim)
       << std::endl << "  - up:  " << VectorToString(m_current_orientation.up);
    Platform::PrintToDebugOutput(ss.str());
#endif
}

} // namespace Methane::Graphics
