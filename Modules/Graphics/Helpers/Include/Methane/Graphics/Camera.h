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

FILE: Methane/Graphics/Camera.h
Camera helper implementation allowing to generate view and projection matrices.

******************************************************************************/

#pragma once

#include "MathTypes.h"

#include <Methane/Data/Types.h>

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
        Vector3f   eye;
        Vector3f   aim;
        Vector3f   up;
    };

    struct Parameters
    {
        float near_depth;
        float far_depth;
        float fov_deg;
    };

    Camera(cml::AxisOrientation axis_orientation = g_axis_orientation);

    void Resize(float width, float height) noexcept;
    void SetProjection(Projection projection) noexcept           { m_projection = projection; }
    void ResetOrientation() noexcept                             { m_current_orientation = m_default_orientation; }
    void SetOrientation(const Orientation& orientation) noexcept { m_current_orientation = m_default_orientation = orientation; }
    void SetParameters(const Parameters& parameters) noexcept    { m_parameters = parameters; }
    void RotateYaw(float deg) noexcept;
    void RotatePitch(float deg) noexcept;

    const Orientation& GetOrientation() const noexcept           { return m_current_orientation; }
    float GetAimDistance() const noexcept                        { return GetAimDistance(m_current_orientation); }
    const Data::Point2f& GetScreenSize() const noexcept          { return m_screen_size; }
    Vector3f GetLookDirection() const noexcept                   { return GetLookDirection(m_current_orientation); }

    void GetViewProjMatrices(Matrix44f& out_view, Matrix44f& out_proj) const noexcept;
    void GetViewMatrix(Matrix44f& out_view) const noexcept       { return GetViewMatrix(out_view, m_current_orientation); }
    void GetProjMatrix(Matrix44f& out_proj) const noexcept;

    Matrix44f GetViewMatrix() const noexcept                     { return GetViewMatrix(m_current_orientation);}
    Matrix44f GetProjMatrix() const noexcept;
    Matrix44f GetViewProjMatrix() const noexcept;

    Vector2f  TransformScreenToProj(const Data::Point2i& screen_pos) const noexcept;
    Vector3f  TransformScreenToView(const Data::Point2i& screen_pos) const noexcept;
    Vector3f  TransformScreenToWorld(const Data::Point2i& screen_pos) const noexcept;
    Vector3f  TransformWorldToView(const Vector3f& world_pos) const noexcept { return TransformWorldToView(world_pos, m_current_orientation); }
    Vector3f  TransformViewToWorld(const Vector3f& view_pos)  const noexcept { return TransformViewToWorld(view_pos,  m_current_orientation);  }
    Vector4f  TransformWorldToView(const Vector4f& world_pos) const noexcept { return TransformWorldToView(world_pos, m_current_orientation); }
    Vector4f  TransformViewToWorld(const Vector4f& view_pos)  const noexcept { return TransformViewToWorld(view_pos,  m_current_orientation); }

    void PrintOrientation();

protected:
    float GetFovAngleY() const noexcept;

    static float GetAimDistance(const Orientation& orientation) noexcept { return (orientation.aim - orientation.eye).length(); }
    static Vector3f GetLookDirection(const Orientation& orientation) noexcept { return orientation.aim - orientation.eye; }

    void GetViewMatrix(Matrix44f& out_view, const Orientation& orientation) const noexcept;
    Matrix44f GetViewMatrix(const Orientation& orientation) const noexcept;

    Vector3f TransformWorldToView(const Vector3f& world_pos, const Orientation& orientation) const noexcept { return TransformWorldToView(Vector4f(world_pos, 1.f), orientation).subvector(3); }
    Vector3f TransformViewToWorld(const Vector3f& view_pos, const Orientation& orientation) const noexcept  { return TransformViewToWorld(Vector4f(view_pos,  1.f), orientation).subvector(3); }
    Vector4f TransformWorldToView(const Vector4f& world_pos, const Orientation& orientation) const noexcept;
    Vector4f TransformViewToWorld(const Vector4f& view_pos, const Orientation& orientation) const noexcept;

    const cml::AxisOrientation m_axis_orientation;

    Projection      m_projection            = Projection::Perspective;
    Data::Point2f   m_screen_size           = { 1.f, 1.f };
    float           m_aspect_ratio          = 1.0f;
    Parameters      m_parameters            = { 0.01f, 125.f, 90.f };
    Orientation     m_default_orientation   = { { 15.0f, 15.0f, -15.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
    Orientation     m_current_orientation   = { };
};

} // namespace Methane::Graphics
