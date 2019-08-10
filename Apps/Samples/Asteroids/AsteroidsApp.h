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

FILE: AsteroidsApp.h
Sample demonstrating parallel redering of the distinct asteroids massive

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/Graphics/Mesh.h>

namespace Methane
{
namespace Samples
{

namespace gfx = Methane::Graphics;
namespace pal = Methane::Platform;

struct AsteroidsFrame final : gfx::AppFrame
{
    struct PassResources
    {
        struct MeshResources
        {
            gfx::Buffer::Ptr                    sp_uniforms_buffer;
            gfx::Program::ResourceBindings::Ptr sp_resource_bindings;
        };

        MeshResources               cube;
        MeshResources               floor;
        gfx::Texture::Ptr           sp_rt_texture;
        gfx::RenderPass::Ptr        sp_pass;
        gfx::RenderCommandList::Ptr sp_cmd_list;
    };

    PassResources    shadow_pass;
    PassResources    final_pass;
    gfx::Buffer::Ptr sp_scene_uniforms_buffer;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<AsteroidsFrame>;
class AsteroidsApp final : public GraphicsApp
{
public:
    AsteroidsApp();
    virtual ~AsteroidsApp() override;

    // NativeApp
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    void Update() override;
    void Render() override;

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

    struct SHADER_STRUCT_ALIGN SceneUniforms
    {
        SHADER_FIELD_ALIGN gfx::Vector4f  eye_position;
        SHADER_FIELD_ALIGN gfx::Vector3f  light_position;
    };

    struct SHADER_STRUCT_ALIGN MeshUniforms
    {
        SHADER_FIELD_ALIGN gfx::Matrix44f model_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f mvp_matrix;
        SHADER_FIELD_ALIGN gfx::Matrix44f shadow_mvpx_matrix;
    };

    struct MeshBuffers
    {
        gfx::Buffer::Ptr sp_vertex;
        gfx::Buffer::Ptr sp_index;
        MeshUniforms     uniforms = { };

        template<typename VType>
        void Init(const gfx::BaseMesh<VType>& mesh_data, gfx::Context& context, const std::string& base_name);
    };

    struct RenderPass
    {
        gfx::Program::Ptr       sp_program;
        gfx::RenderState::Ptr   sp_state;
        std::string             command_group_name;
        bool                    is_final_pass = false;
    };

    void RenderScene(const RenderPass& render_pass, AsteroidsFrame::PassResources& render_pass_data, gfx::Texture& shadow_texture, bool is_shadow_rendering);

    const gfx::BoxMesh<Vertex>  m_cube_mesh;
    const gfx::RectMesh<Vertex> m_floor_mesh;
    const float                 m_scene_scale;
    const Constants             m_scene_constants;

    gfx::ActionCamera           m_scene_camera;
    gfx::ActionCamera           m_light_camera;
    gfx::Buffer::Ptr            m_sp_const_buffer;
    gfx::Texture::Ptr           m_sp_cube_texture;
    gfx::Texture::Ptr           m_sp_floor_texture;
    gfx::Sampler::Ptr           m_sp_texture_sampler;
    gfx::Sampler::Ptr           m_sp_shadow_sampler;

    SceneUniforms               m_scene_uniforms = { };
    MeshBuffers                 m_cube_buffers;
    MeshBuffers                 m_floor_buffers;
    RenderPass                  m_shadow_pass;
    RenderPass                  m_final_pass;
};

} // namespace Samples
} // namespace Methane
