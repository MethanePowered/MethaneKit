/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: TexturedCubeApp.h
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>

namespace hlslpp // NOSONAR
{
#pragma pack(push, 16)
#include "Shaders/TexturedCubeUniforms.h" // NOSONAR
#pragma pack(pop)
}

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

struct TexturedCubeFrame final : Graphics::AppFrame
{
    rhi::ProgramBindings          program_bindings;
    rhi::IProgramArgumentBinding* uniforms_binding_ptr = nullptr;
    rhi::RenderCommandList        render_cmd_list;
    rhi::CommandListSet           execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<TexturedCubeFrame>;

class TexturedCubeApp final // NOSONAR - destructor required
    : public UserInterfaceApp
{
public:
    TexturedCubeApp();
    ~TexturedCubeApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

protected:
    // IContextCallback override
    void OnContextReleased(rhi::IContext& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);

    hlslpp::Uniforms m_shader_uniforms { };
    gfx::Camera      m_camera;
    rhi::RenderState m_render_state;
    rhi::BufferSet   m_vertex_buffer_set;
    rhi::Buffer      m_index_buffer;
    rhi::Texture     m_cube_texture;
    rhi::Sampler     m_texture_sampler;
};

} // namespace Methane::Tutorials
