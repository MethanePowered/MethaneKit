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

namespace hlslpp // NOSONAR
{
#define ENABLE_SHADOWS
#pragma pack(push, 16)
#include "Shaders/ShadowCubeUniforms.h" // NOSONAR
#pragma pack(pop)
}

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

struct ShadowCubeFrame final
    : gfx::AppFrame
{
    struct PassResources
    {
        struct MeshResources
        {
            rhi::Buffer          uniforms_buffer;
            rhi::ProgramBindings program_bindings;
        };

        MeshResources          cube;
        MeshResources          floor;
        rhi::Texture           rt_texture;
        rhi::RenderPass        render_pass;
        rhi::RenderCommandList cmd_list;
    };

    PassResources       shadow_pass;
    PassResources       final_pass;
    rhi::Buffer         scene_uniforms_buffer;
    rhi::CommandListSet execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<ShadowCubeFrame>;

class ShadowCubeApp final // NOSONAR - destructor required
    : public UserInterfaceApp
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
    void OnContextReleased(rhi::IContext& context) override;

private:
    using TexturedMeshBuffersBase = gfx::TexturedMeshBuffers<hlslpp::MeshUniforms>;
    class TexturedMeshBuffers : public TexturedMeshBuffersBase
    {
    public:
        using TexturedMeshBuffersBase::TexturedMeshBuffersBase;

        void SetShadowPassUniforms(hlslpp::MeshUniforms&& uniforms) noexcept { m_shadow_pass_uniforms = std::move(uniforms); }

        [[nodiscard]] const hlslpp::MeshUniforms& GetShadowPassUniforms() const noexcept            { return m_shadow_pass_uniforms; }
        [[nodiscard]] const rhi::SubResource&     GetShadowPassUniformsSubresource() const noexcept { return m_shadow_pass_uniforms_subresource; }

    private:
        hlslpp::MeshUniforms   m_shadow_pass_uniforms{};
        const rhi::SubResource m_shadow_pass_uniforms_subresource{
            reinterpret_cast<Data::ConstRawPtr>(&m_shadow_pass_uniforms), // NOSONAR
            sizeof(hlslpp::MeshUniforms)
        };
    };

    struct RenderPassState
    {
        RenderPassState(bool is_final_pass, const std::string& command_group_name);
        void Release();

        const bool                       is_final_pass;
        const rhi::CommandListDebugGroup debug_group;
        rhi::RenderState                 render_state;
        rhi::ViewState                   view_state;
    };

    bool Animate(double elapsed_seconds, double delta_seconds);
    void RenderScene(const RenderPassState& render_pass, const ShadowCubeFrame::PassResources& render_pass_resources) const;

    const float                 m_scene_scale = 15.F;
    const hlslpp::Constants     m_scene_constants{
        { 1.F, 1.F, 0.74F, 1.F }, // - light_color
        700.F,                    // - light_power
        0.04F,                    // - light_ambient_factor
        30.F                      // - light_specular_factor
    };
    hlslpp::SceneUniforms    m_scene_uniforms{ };
    rhi::SubResource         m_scene_uniforms_subresource{
        reinterpret_cast<Data::ConstRawPtr>(&m_scene_uniforms), // NOSONAR
        sizeof(hlslpp::SceneUniforms)
    };
    gfx::Camera              m_view_camera;
    gfx::Camera              m_light_camera;
    rhi::Buffer              m_const_buffer;
    rhi::Sampler             m_texture_sampler;
    rhi::Sampler             m_shadow_sampler;
    Ptr<TexturedMeshBuffers> m_cube_buffers_ptr;
    Ptr<TexturedMeshBuffers> m_floor_buffers_ptr;
    rhi::RenderPattern       m_shadow_pass_pattern;
    RenderPassState          m_shadow_pass { false, "Shadow Render Pass" };
    RenderPassState          m_final_pass  { true,  "Final Render Pass" };
};

} // namespace Methane::Tutorials
