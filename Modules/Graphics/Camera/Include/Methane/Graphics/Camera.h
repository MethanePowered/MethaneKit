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

#include <hlsl++.h>
#include <optional>

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
        hlslpp::float3 eye;
        hlslpp::float3 aim;
        hlslpp::float3 up;
    };

    struct Parameters
    {
        float near_depth;
        float far_depth;
        float fov_deg;
    };

    explicit Camera() noexcept;

    void Resize(const Data::FloatSize& screen_size);
    void Resize(const Data::FrameSize& screen_size) { Resize(static_cast<Data::FloatSize>(screen_size)); }
    void SetProjection(Projection projection);
    void SetParameters(const Parameters& parameters);

    inline void ResetOrientation() noexcept                                 { m_current_orientation = m_default_orientation; m_is_current_view_matrix_dirty = true; }
    inline void ResetOrientation(const Orientation& orientation) noexcept   { m_current_orientation = m_default_orientation = orientation; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientation(const Orientation& orientation) noexcept     { m_current_orientation = orientation; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientationEye(const hlslpp::float3& eye) noexcept       { m_current_orientation.eye = eye; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientationAim(const hlslpp::float3& aim) noexcept       { m_current_orientation.aim = aim; m_is_current_view_matrix_dirty = true; }
    inline void SetOrientationUp(const hlslpp::float3& up) noexcept         { m_current_orientation.up = up;   m_is_current_view_matrix_dirty = true; }
    void        Rotate(const hlslpp::float3& axis, float deg) noexcept;

    inline const Data::FloatSize& GetScreenSize() const noexcept            { return m_screen_size; }
    inline const Orientation& GetOrientation() const noexcept               { return m_current_orientation; }
    inline float GetAimDistance() const noexcept                            { return GetAimDistance(m_current_orientation); }
    inline hlslpp::float3 GetLookDirection() const noexcept                 { return GetLookDirection(m_current_orientation); }

    const hlslpp::float4x4& GetViewMatrix() const noexcept;
    const hlslpp::float4x4& GetProjMatrix() const;
    const hlslpp::float4x4& GetViewProjMatrix() const noexcept;

    hlslpp::float2 TransformScreenToProj(const Data::Point2I& screen_pos) const noexcept;
    hlslpp::float3 TransformScreenToView(const Data::Point2I& screen_pos) const noexcept;
    hlslpp::float3 TransformScreenToWorld(const Data::Point2I& screen_pos) const noexcept;
    hlslpp::float3 TransformWorldToView(const hlslpp::float3& world_pos) const noexcept { return TransformWorldToView(world_pos, m_current_orientation); }
    hlslpp::float3 TransformViewToWorld(const hlslpp::float3& view_pos)  const noexcept { return TransformViewToWorld(view_pos,  m_current_orientation); }
    hlslpp::float4 TransformWorldToView(const hlslpp::float4& world_pos) const noexcept { return TransformWorldToView(world_pos, m_current_orientation); }
    hlslpp::float4 TransformViewToWorld(const hlslpp::float4& view_pos)  const noexcept { return TransformViewToWorld(view_pos,  m_current_orientation); }

    std::string GetOrientationString() const;

protected:
    float GetFovAngleY() const noexcept;

    static inline hlslpp::float3 GetLookDirection(const Orientation& orientation) noexcept   { return orientation.aim - orientation.eye; }
    static inline float    GetAimDistance(const Orientation& orientation) noexcept     { return hlslpp::length(GetLookDirection(orientation)); }

    hlslpp::float4x4 CreateViewMatrix(const Orientation& orientation) const noexcept;
    hlslpp::float4x4 CreateProjMatrix() const;

    hlslpp::float3 TransformWorldToView(const hlslpp::float3& world_pos, const Orientation& orientation) const noexcept { return TransformWorldToView(hlslpp::float4(world_pos, 1.F), orientation).xyz; }
    hlslpp::float3 TransformViewToWorld(const hlslpp::float3& view_pos, const Orientation& orientation) const noexcept  { return TransformViewToWorld(hlslpp::float4(view_pos,  1.F), orientation).xyz; }
    hlslpp::float4 TransformWorldToView(const hlslpp::float4& world_pos, const Orientation& orientation) const noexcept;
    hlslpp::float4 TransformViewToWorld(const hlslpp::float4& view_pos, const Orientation& orientation) const noexcept;

private:
    hlslpp::frustum CreateFrustum() const;
    void UpdateProjectionSettings();

    Projection        m_projection            = Projection::Perspective;
    Data::FloatSize   m_screen_size           { 1.F, 1.F };
    float             m_aspect_ratio          = 1.0F;
    Parameters        m_parameters            { 0.01F, 125.F, 90.F };
    Orientation       m_default_orientation   { { 15.0F, 15.0F, -15.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    Orientation       m_current_orientation   { };

    std::optional<hlslpp::projection> m_projection_settings;
    mutable hlslpp::float4x4 m_current_view_matrix;
    mutable hlslpp::float4x4 m_current_proj_matrix;
    mutable hlslpp::float4x4 m_current_view_proj_matrix;
    mutable bool             m_is_current_view_matrix_dirty = true;
    mutable bool             m_is_current_proj_matrix_dirty = true;
};

} // namespace Methane::Graphics
