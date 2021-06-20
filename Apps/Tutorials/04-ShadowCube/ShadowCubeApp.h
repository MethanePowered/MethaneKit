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

FILE: ShadowCubeApp.h
Tutorial demonstrating shadow-pass rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct ShadowCubeFrame final : gfx::AppFrame
{
    struct PassResources
    {
        struct MeshResources
        {
            Ptr<gfx::Buffer>          uniforms_buffer_ptr;
            Ptr<gfx::ProgramBindings> program_bindings_ptr;
        };

        MeshResources               cube;
        MeshResources               floor;
        Ptr<gfx::Texture>           rt_texture_ptr;
        Ptr<gfx::RenderPass>        render_pass_ptr;
        Ptr<gfx::RenderCommandList> cmd_list_ptr;
    };

    PassResources            shadow_pass;
    PassResources            final_pass;
    Ptr<gfx::Buffer>         scene_uniforms_buffer_ptr;
    Ptr<gfx::CommandListSet> execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<ShadowCubeFrame>;

class ShadowCubeApp final : public UserInterfaceApp
{
public:
    ShadowCubeApp();
    ~ShadowCubeApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

protected:
    // IContextCallback override
    void OnContextReleased(gfx::Context& context) override;

private:
    struct META_UNIFORM_ALIGN Constants
    {
        hlslpp::float4 light_color;
        float          light_power;
        float          light_ambient_factor;
        float          light_specular_factor;
    };

    struct META_UNIFORM_ALIGN SceneUniforms
    {
        hlslpp::float4 eye_position;
        hlslpp::float3 light_position;
    };

    struct META_UNIFORM_ALIGN MeshUniforms
    {
        hlslpp::float4x4 model_matrix;
        hlslpp::float4x4 mvp_matrix;
        hlslpp::float4x4 shadow_mvpx_matrix;
    };

    using TexturedMeshBuffersBase = gfx::TexturedMeshBuffers<MeshUniforms>;
    class TexturedMeshBuffers : public TexturedMeshBuffersBase
    {
    public:
        using TexturedMeshBuffersBase::TexturedMeshBuffersBase;

        void SetShadowPassUniforms(MeshUniforms&& uniforms) noexcept { m_shadow_pass_uniforms = std::move(uniforms); }

        [[nodiscard]] const MeshUniforms&                GetShadowPassUniforms() const noexcept               { return m_shadow_pass_uniforms; }
        [[nodiscard]] const gfx::Resource::SubResources& GetShadowPassUniformsSubresources() const noexcept   { return m_shadow_pass_uniforms_subresources; }

    private:
        MeshUniforms                m_shadow_pass_uniforms{};
        gfx::Resource::SubResources m_shadow_pass_uniforms_subresources{
            { reinterpret_cast<Data::ConstRawPtr>(&m_shadow_pass_uniforms), sizeof(MeshUniforms) }
        };
    };

    struct RenderPassState
    {
        RenderPassState(bool is_final_pass, const std::string& command_group_name);
        void Release();

        const bool                              is_final_pass;
        const Ptr<gfx::CommandList::DebugGroup> debug_group_ptr;
        Ptr<gfx::RenderState>                   render_state_ptr;
        Ptr<gfx::ViewState>                     view_state_ptr;
    };

    bool Animate(double elapsed_seconds, double delta_seconds);
    void RenderScene(const RenderPassState& render_pass, const ShadowCubeFrame::PassResources& render_pass_resources) const;

    const float                 m_scene_scale = 15.F;
    const Constants             m_scene_constants{
        { 1.F, 1.F, 0.74F, 1.F }, // - light_color
        700.F,                    // - light_power
        0.04F,                    // - light_ambient_factor
        30.F                      // - light_specular_factor
    };
    SceneUniforms               m_scene_uniforms{ };
    gfx::Resource::SubResources m_scene_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), sizeof(SceneUniforms) }
    };
    gfx::Camera                 m_view_camera;
    gfx::Camera                 m_light_camera;
    Ptr<gfx::Buffer>            m_const_buffer_ptr;
    Ptr<gfx::Sampler>           m_texture_sampler_ptr;
    Ptr<gfx::Sampler>           m_shadow_sampler_ptr;
    Ptr<TexturedMeshBuffers>    m_cube_buffers_ptr;
    Ptr<TexturedMeshBuffers>    m_floor_buffers_ptr;
    Ptr<gfx::RenderPattern>     m_shadow_pass_pattern_ptr;
    RenderPassState             m_shadow_pass { false, "Shadow Render Pass" };
    RenderPassState             m_final_pass  { true,  "Final Render Pass" };
};

} // namespace Methane::Tutorials
