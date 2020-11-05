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

FILE: Methane/Graphics/Camera.h
Camera helper implementation allowing to generate view and projection matrices.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>

#include <cml/mathlib/constants.h>

namespace Methane::Graphics
{

class Camera
{
public:
    enum class Projection
    {
        Perspective,
        Orthogonal
    };

    struct Orientation
    {
        Vector3f eye;
        Vector3f aim;
        Vector3f up;
    };

    struct Parameters
    {
        float near_depth;
        float far_depth;
        float fov_deg;
    };

    Camera(cml::AxisOrientation axis_orientation = g_axis_orientation) noexcept;

    inline void Resize(const Data::FloatSize& screen_size) noexcept         { m_screen_size = screen_size; m_aspect_ratio = screen_size.width / screen_size.height; m_is_current_proj_matrix_dirty = true; }
    inline void SetProjection(Projection projection) noexcept               { m_projection = projection; m_is_current_proj_matrix_dirty = true; }
    inline void SetParameters(const Parameters& parameters) noexcept        { m_parameters = parameters; m_is_current_proj_matrix_dirty = true; }
    inline void ResetOrientation() noexcept                                 { m_current_orientation = m_default_orientation; m_is_current_view_matrix_dirty = true; }
    inline void ResetOrientation(const Orientation& orientation) noexcept   { m_current_orientation = m_default_orientation = orientation; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientation(const Orientation& orientation) noexcept     { m_current_orientation = orientation; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientationEye(const Vector3f& eye) noexcept             { m_current_orientation.eye = eye; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientationAim(const Vector3f& aim) noexcept             { m_current_orientation.aim = aim; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientationUp(const Vector3f& up) noexcept               { m_current_orientation.up = up;   m_is_current_view_matrix_dirty = true; }
    void        Rotate(const Vector3f& axis, float deg) noexcept;

    inline const Data::FloatSize& GetScreenSize() const noexcept            { return m_screen_size; }
    inline const Orientation& GetOrientation() const noexcept               { return m_current_orientation; }
    inline float GetAimDistance() const noexcept                            { return GetAimDistance(m_current_orientation); }
    inline Vector3f GetLookDirection() const noexcept                       { return GetLookDirection(m_current_orientation); }

    const Matrix44f& GetViewMatrix() const noexcept;
    const Matrix44f& GetProjMatrix() const;
    const Matrix44f& GetViewProjMatrix() const noexcept;

    Vector2f TransformScreenToProj(const Data::Point2i& screen_pos) const noexcept;
    Vector3f TransformScreenToView(const Data::Point2i& screen_pos) const noexcept;
    Vector3f TransformScreenToWorld(const Data::Point2i& screen_pos) const noexcept;
    Vector3f TransformWorldToView(const Vector3f& world_pos) const noexcept { return TransformWorldToView(world_pos, m_current_orientation); }
    Vector3f TransformViewToWorld(const Vector3f& view_pos)  const noexcept { return TransformViewToWorld(view_pos,  m_current_orientation); }
    Vector4f TransformWorldToView(const Vector4f& world_pos) const noexcept { return TransformWorldToView(world_pos, m_current_orientation); }
    Vector4f TransformViewToWorld(const Vector4f& view_pos)  const noexcept { return TransformViewToWorld(view_pos,  m_current_orientation); }

    std::string GetOrientationString() const;

protected:
    float GetFovAngleY() const noexcept;

    static inline Vector3f GetLookDirection(const Orientation& orientation) noexcept   { return orientation.aim - orientation.eye; }
    static inline float    GetAimDistance(const Orientation& orientation) noexcept     { return GetLookDirection(orientation).length(); }

    Matrix44f GetViewMatrix(const Orientation& orientation) const noexcept;
    void GetViewMatrix(Matrix44f& out_view, const Orientation& orientation) const noexcept;
    void GetViewMatrix(Matrix44f& out_view) const noexcept { return GetViewMatrix(out_view, m_current_orientation); }
    void GetProjMatrix(Matrix44f& out_proj) const;

    Vector3f TransformWorldToView(const Vector3f& world_pos, const Orientation& orientation) const noexcept { return TransformWorldToView(Vector4f(world_pos, 1.F), orientation).subvector(3); }
    Vector3f TransformViewToWorld(const Vector3f& view_pos, const Orientation& orientation) const noexcept  { return TransformViewToWorld(Vector4f(view_pos,  1.F), orientation).subvector(3); }
    Vector4f TransformWorldToView(const Vector4f& world_pos, const Orientation& orientation) const noexcept;
    Vector4f TransformViewToWorld(const Vector4f& view_pos, const Orientation& orientation) const noexcept;

private:
    const cml::AxisOrientation m_axis_orientation;

    Projection        m_projection            = Projection::Perspective;
    Data::FloatSize   m_screen_size           { 1.F, 1.F };
    float             m_aspect_ratio          = 1.0F;
    Parameters        m_parameters            { 0.01F, 125.F, 90.F };
    Orientation       m_default_orientation   { { 15.0F, 15.0F, -15.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    Orientation       m_current_orientation   { };

    mutable Matrix44f m_current_view_matrix;
    mutable Matrix44f m_current_proj_matrix;
    mutable Matrix44f m_current_view_proj_matrix;
    mutable bool      m_is_current_view_matrix_dirty = true;
    mutable bool      m_is_current_proj_matrix_dirty = true;
};

} // namespace Methane::Graphics
