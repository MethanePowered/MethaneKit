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

FILE: Methane/Graphics/Camera.cpp
Camera helper implementation allowing to generate view and projectrion matrices.

******************************************************************************/

#include <Methane/Graphics/Camera.h>

#include <cml/mathlib/mathlib.h>

using namespace Methane::Graphics;

Camera::Camera(cml::AxisOrientation axis_orientation)
    : m_axis_orientation(axis_orientation)
{
    ResetOrientaion();
}

void Camera::RotateYaw(float deg) noexcept
{
    Matrix33f rotation_matrix;
    cml::matrix_rotation_axis_angle(rotation_matrix, m_current_orientation.up, cml::rad(deg));
    m_current_orientation.eye = m_current_orientation.eye * rotation_matrix;
}

void Camera::RotatePitch(float deg) noexcept
{
    Matrix33f rotation_matrix;
    auto right = cml::cross(m_current_orientation.eye, m_current_orientation.up).normalize();
    cml::matrix_rotation_axis_angle(rotation_matrix, right, cml::rad(deg));
    m_current_orientation.eye = m_current_orientation.eye * rotation_matrix;
}

void Camera::GetViewProjMatrices(Matrix44f& out_view, Matrix44f& out_proj) const noexcept
{
    GetViewMatrix(out_view);
    GetProjMatrix(out_proj);
}

void Camera::GetViewMatrix(Matrix44f& out_view) const noexcept
{
    cml::matrix_look_at(out_view, m_current_orientation.eye, m_current_orientation.aim, m_current_orientation.up, m_axis_orientation);
}

void Camera::GetProjMatrix(Matrix44f& out_proj) const noexcept
{
    switch (m_projection)
    {
    case Projection::Perspective:
        cml::matrix_perspective_yfov(out_proj, GetFOVAngleY(), m_aspect_ratio, m_parameters.near_depth, m_parameters.far_depth, m_axis_orientation, cml::ZClip::z_clip_neg_one);
        break;
    case Projection::Orthogonal:
        cml::matrix_orthographic(out_proj, m_width, m_height, m_parameters.near_depth, m_parameters.far_depth, m_axis_orientation, cml::ZClip::z_clip_neg_one);
        break;
    }
}

Matrix44f Camera::GetViewMatrix() const noexcept
{
    Matrix44f view_matrix;
    GetViewMatrix(view_matrix);
    return view_matrix;
}

Matrix44f Camera::GetProjMatrix() const noexcept
{
    Matrix44f proj_matrix;
    GetProjMatrix(proj_matrix);
    return proj_matrix;
}

Matrix44f Camera::GetViewProjMatrix() const noexcept
{
    return GetViewMatrix() * GetProjMatrix();
}

float Camera::GetFOVAngleY() const noexcept
{
    float fov_angle_y = m_parameters.fov_deg * cml::constants<float>::pi() / 180.0f;
    if (m_aspect_ratio != 0.f && m_aspect_ratio < 1.0f)
    {
        fov_angle_y /= m_aspect_ratio;
    }
    return fov_angle_y;
}
