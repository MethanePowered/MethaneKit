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
        float              scale;
        const gfx::Camera& view_camera;
        std::string        texture_path;
        bool               mipmapped = false;
        float              lod_bias = 0.f;
    };

    Planet(gfx::Context& context, gfx::ImageLoader& image_loader, const Settings& settings);

    gfx::Program::ResourceBindings::Ptr CreateResourceBindings(const gfx::Buffer::Ptr& sp_uniforms_buffer);
    void Resize(const gfx::FrameSize& frame_size);
    void Update();
    void Draw(gfx::RenderCommandList& cmd_list, gfx::Buffer& uniforms_buffer, gfx::Program::ResourceBindings& resource_bindings);

private:
    struct SHADER_STRUCT_ALIGN Constants
    {
        SHADER_FIELD_ALIGN gfx::Color4f   light_color;
        SHADER_FIELD_PACK  float          light_power;
        SHADER_FIELD_PACK  float          light_ambient_factor;
        SHADER_FIELD_PACK  float          light_specular_factor;
    };

    struct SHADER_STRUCT_ALIGN Uniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
        SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
    };

    using TexturedMeshBuffers = gfx::TexturedMeshBuffers<Uniforms>;

    Settings                    m_settings;
    gfx::Context&               m_context;
    TexturedMeshBuffers         m_mesh_buffers;
    gfx::Sampler::Ptr           m_sp_texture_sampler;
    gfx::RenderState::Ptr       m_sp_state;
    gfx::RenderCommandList::Ptr m_sp_command_list;
};

} // namespace Methane::Graphics
