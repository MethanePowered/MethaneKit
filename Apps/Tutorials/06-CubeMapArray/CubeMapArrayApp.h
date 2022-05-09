/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: CubeMapArrayApp.h
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/CubeMapArrayUniforms.h" // NOSONAR
#pragma pack(pop)
}

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct CubeMapArrayFrame final : Graphics::AppFrame
{
    Ptr<gfx::Buffer>            uniforms_buffer_ptr;
    Ptr<gfx::ProgramBindings>   program_bindings_ptr;
    Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>    execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<CubeMapArrayFrame>;

class CubeMapArrayApp final : public UserInterfaceApp // NOSONAR
{
public:
    CubeMapArrayApp();
    ~CubeMapArrayApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

protected:
    // IContextCallback override
    void OnContextReleased(gfx::Context& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);

    Ptr<gfx::CommandListSet> RenderFaceTextures(gfx::Texture& texture);

    hlslpp::float4x4      m_model_matrix;
    hlslpp::Uniforms      m_shader_uniforms { };
    gfx::Camera           m_camera;
    Ptr<gfx::RenderState> m_render_state_ptr;
    Ptr<gfx::BufferSet>   m_vertex_buffer_set_ptr;
    Ptr<gfx::Buffer>      m_index_buffer_ptr;
    Ptr<gfx::Texture>     m_cube_map_array_texture_ptr;
    Ptr<gfx::Sampler>     m_texture_sampler_ptr;

    const gfx::Resource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) } // NOSONAR
    };
};

} // namespace Methane::Tutorials
