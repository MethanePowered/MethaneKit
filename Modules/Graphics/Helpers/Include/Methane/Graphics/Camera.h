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

FILE: Methane/Graphics/Camera.h
Camera helper implementation allowing to generate view and projectrion matrices.

******************************************************************************/

#pragma once

#include "MathTypes.h"
#include <cml/mathlib/constants.h>

namespace Methane
{
namespace Graphics
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
        Vector3f   at;
        Vector3f   up;
    };

    struct Parameters
    {
        float near_depth;
        float far_depth;
        float fov_deg;
    };

    Camera(cml::AxisOrientation axis_orientation = g_axis_orientation);

    void SetProjection(Projection projection) noexcept           { m_projection = projection; }
    void Resize(float width, float height) noexcept              { m_width = width; m_height = height; m_aspect_ratio = width / height; }
    void ResetOrientaion() noexcept                              { m_current_orientation = m_default_orientation; }
    void SetOrientation(const Orientation& orientation) noexcept { m_current_orientation = m_default_orientation = orientation; }
    void SetParamters(const Parameters& parameters) noexcept     { m_parameters = parameters; }
    void RotateYaw(float deg) noexcept;
    void RotatePitch(float deg) noexcept;

    const Orientation& GetOrientation() const noexcept           { return m_current_orientation; }
    void GetViewProjMatrices(Matrix44f& out_view, Matrix44f& out_proj) const noexcept;
    void GetViewMatrix(Matrix44f& out_view) const noexcept;
    void GetProjMatrix(Matrix44f& out_proj) const noexcept;
    Matrix44f GetViewMatrix() const noexcept;
    Matrix44f GetProjMatrix() const noexcept;
    Matrix44f GetViewProjMatrix() const noexcept;

protected:
    float GetFOVAngleY() const noexcept;

    const cml::AxisOrientation m_axis_orientation;

    Projection      m_projection            = Projection::Perspective;
    float           m_width                 = 1.0f;
    float           m_height                = 1.0f;
    float           m_aspect_ratio          = 1.0f;
    Parameters      m_parameters            = { 0.01f, 125.f, 90.f };
    Orientation     m_default_orientation   = { { 15.0f, 15.0f, -15.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
    Orientation     m_current_orientation;
};

} // namespace Graphics
} // namespace Methane
