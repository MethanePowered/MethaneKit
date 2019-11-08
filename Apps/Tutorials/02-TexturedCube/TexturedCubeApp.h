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

FILE: TexturedCubeApp.h
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Graphics/Kit.h>

#include <string>
#include <vector>
#include <array>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace dat = Methane::Data;

struct TexturedCubeFrame final : gfx::AppFrame
{
    gfx::Buffer::Ptr                    sp_uniforms_buffer;
    gfx::Program::ResourceBindings::Ptr sp_resource_bindings;
    gfx::RenderCommandList::Ptr         sp_cmd_list;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<TexturedCubeFrame>;

class TexturedCubeApp final : public GraphicsApp
{
public:
    TexturedCubeApp();
    ~TexturedCubeApp() override;

    // App interface
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    void Update() override;
    void Render() override;

    // Context::Callback interface
    void OnContextReleased() override;

private:
    struct Vertex
    {
        gfx::Mesh::Position position;
        gfx::Mesh::Normal   normal;
        gfx::Mesh::TexCoord texcoord;

        using FieldsArray = std::array<gfx::Mesh::VertexField, 3>;
        static constexpr const FieldsArray layout = {
            gfx::Mesh::VertexField::Position,
            gfx::Mesh::VertexField::Normal,
            gfx::Mesh::VertexField::TexCoord,
        };
    };

    struct SHADER_STRUCT_ALIGN Constants
    {
        SHADER_FIELD_ALIGN gfx::Color     light_color;
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

    const Constants         m_shader_constants;
    Uniforms                m_shader_uniforms;
    gfx::Camera             m_camera;
    gfx::BoxMesh<Vertex>    m_cube_mesh;
    float                   m_cube_scale;
    gfx::Program::Ptr       m_sp_program;
    gfx::RenderState::Ptr   m_sp_state;
    gfx::Buffer::Ptr        m_sp_vertex_buffer;
    gfx::Buffer::Ptr        m_sp_index_buffer;
    gfx::Buffer::Ptr        m_sp_const_buffer;
    gfx::Texture::Ptr       m_sp_cube_texture;
    gfx::Sampler::Ptr       m_sp_texture_sampler;
    
};

} // namespace Methane::Tutorials
