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
        struct ProgramBindings
        {
            rhi::ProgramBindings          program_bindings;
            rhi::IProgramArgumentBinding* scene_uniforms_binding_ptr = nullptr;
            rhi::IProgramArgumentBinding* mesh_uniforms_binding_ptr  = nullptr;
        };

        rhi::Texture           rt_texture;
        rhi::RenderPass        render_pass;
        rhi::RenderCommandList cmd_list;
        ProgramBindings        cube_bindings;
        ProgramBindings        floor_bindings;
    };

    PassResources       shadow_pass;
    PassResources       final_pass;
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
    using TexturedMeshBuffers = gfx::TexturedMeshBuffers<hlslpp::MeshUniforms>;

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

    gfx::Camera              m_view_camera;
    gfx::Camera              m_light_camera;
    rhi::Sampler             m_texture_sampler;
    rhi::Sampler             m_shadow_sampler;
    Ptr<TexturedMeshBuffers> m_cube_buffers_ptr;
    Ptr<TexturedMeshBuffers> m_floor_buffers_ptr;
    rhi::RenderPattern       m_shadow_pass_pattern;
    RenderPassState          m_shadow_pass { false, "Shadow Render Pass" };
    RenderPassState          m_final_pass  { true,  "Final Render Pass" };
};

} // namespace Methane::Tutorials
