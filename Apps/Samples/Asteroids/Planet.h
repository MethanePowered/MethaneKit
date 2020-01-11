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

FILE: Planet.h
Planet rendering primitive

******************************************************************************/

#pragma once

#include <Methane/Graphics/MeshBuffers.hpp>
#include <Methane/Graphics/Context.h>
#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/RenderState.h>
#include <Methane/Graphics/Buffer.h>
#include <Methane/Graphics/Program.h>
#include <Methane/Graphics/Sampler.h>
#include <Methane/Graphics/MathTypes.h>
#include <Methane/Graphics/Types.h>

#include <memory>

namespace Methane::Samples
{

namespace gfx = Graphics;

class Planet
{
public:
    using Ptr = std::shared_ptr<Planet>;

    struct Settings
    {
        const gfx::Camera& view_camera;
        const gfx::Camera& light_camera;
        std::string        texture_path;
        gfx::Vector3f      position;
        float              scale;
        float              spin_velocity_rps = 0.3f; // (rps = radians per second)
        bool               depth_reversed = false;
        bool               mipmapped = false;
        float              lod_bias = 0.f;
    };

    struct SHADER_STRUCT_ALIGN Uniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
        SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
    };

    Planet(gfx::Context& context, gfx::ImageLoader& image_loader, const Settings& settings);

    gfx::Program::ResourceBindings::Ptr CreateResourceBindings(const gfx::Buffer::Ptr& sp_constants_buffer, const gfx::Buffer::Ptr& sp_uniforms_buffer);
    void Resize(const gfx::FrameSize& frame_size);
    bool Update(double elapsed_seconds, double delta_seconds);
    void Draw(gfx::RenderCommandList& cmd_list, gfx::MeshBufferBindings& buffer_bindings);

private:
    using TexturedMeshBuffers = gfx::TexturedMeshBuffers<Uniforms>;

    Settings                    m_settings;
    gfx::Context&               m_context;
    TexturedMeshBuffers         m_mesh_buffers;
    gfx::Sampler::Ptr           m_sp_texture_sampler;
    gfx::RenderState::Ptr       m_sp_state;
};

} // namespace Methane::Graphics
