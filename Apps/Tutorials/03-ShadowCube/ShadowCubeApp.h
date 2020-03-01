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

FILE: ShadowCubeApp.h
Tutorial demonstrating shadow-pass rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Graphics/Kit.h>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace dat = Methane::Data;

struct ShadowCubeFrame final : gfx::AppFrame
{
    struct PassResources
    {
        struct MeshResources
        {
            Ptr<gfx::Buffer>          sp_uniforms_buffer;
            Ptr<gfx::ProgramBindings> sp_program_bindings;
        };

        MeshResources               cube;
        MeshResources               floor;
        Ptr<gfx::Texture>           sp_rt_texture;
        Ptr<gfx::RenderPass>        sp_pass;
        Ptr<gfx::RenderCommandList> sp_cmd_list;
    };

    PassResources    shadow_pass;
    PassResources    final_pass;
    Ptr<gfx::Buffer> sp_scene_uniforms_buffer;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<ShadowCubeFrame>;
class ShadowCubeApp final : public GraphicsApp
{
public:
    ShadowCubeApp();
    ~ShadowCubeApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

    // Context::Callback override
    void OnContextReleased() override;

private:
    struct SHADER_STRUCT_ALIGN Constants
    {
        SHADER_FIELD_ALIGN gfx::Color4f   light_color;
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

    using TexturedMeshBuffersBase = gfx::TexturedMeshBuffers<MeshUniforms>;
    class TexturedMeshBuffers : public TexturedMeshBuffersBase
    {
    public:
        using TexturedMeshBuffersBase::TexturedMeshBuffersBase;

        const MeshUniforms& GetShadowPassUniforms() const                       { return m_shadow_pass_uniforms; }
        void                SetShadowPassUniforms(const MeshUniforms& uniforms) { m_shadow_pass_uniforms = uniforms; }

    private:
        MeshUniforms m_shadow_pass_uniforms = {};
    };

    struct RenderPass
    {
        RenderPass(bool is_final_pass, std::string command_group_name)
            : is_final_pass(is_final_pass)
            , command_group_name(std::move(command_group_name))
        { }

        const bool              is_final_pass;
        const std::string       command_group_name;
        Ptr<gfx::RenderState>   sp_state;

        void Release();
    };

    void RenderScene(const RenderPass &render_pass, ShadowCubeFrame::PassResources &render_pass_resources, gfx::Texture &shadow_texture);

    const float                 m_scene_scale;
    const Constants             m_scene_constants;
    SceneUniforms               m_scene_uniforms = { };
    gfx::Camera                 m_view_camera;
    gfx::Camera                 m_light_camera;

    Ptr<gfx::Buffer>            m_sp_const_buffer;
    Ptr<gfx::Sampler>           m_sp_texture_sampler;
    Ptr<gfx::Sampler>           m_sp_shadow_sampler;
    Ptr<TexturedMeshBuffers>    m_sp_cube_buffers;
    Ptr<TexturedMeshBuffers>    m_sp_floor_buffers;
    RenderPass                  m_shadow_pass;
    RenderPass                  m_final_pass;
};

} // namespace Methane::Tutorials
