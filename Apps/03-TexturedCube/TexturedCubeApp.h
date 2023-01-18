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
    rhi::Buffer            uniforms_buffer;
    rhi::ProgramBindings   program_bindings;
    rhi::RenderCommandList render_cmd_list;
    rhi::CommandListSet    execute_cmd_list_set;

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

    const float             m_cube_scale = 15.F;
    const hlslpp::Constants m_shader_constants{
        { 1.F, 1.F, 0.74F, 1.F },  // - light_color
        700.F,                     // - light_power
        0.04F,                     // - light_ambient_factor
        30.F                       // - light_specular_factor
    };
    hlslpp::Uniforms m_shader_uniforms { };
    gfx::Camera      m_camera;
    rhi::RenderState m_render_state;
    rhi::BufferSet   m_vertex_buffer_set;
    rhi::Buffer      m_index_buffer;
    rhi::Buffer      m_const_buffer;
    rhi::Texture     m_cube_texture;
    rhi::Sampler     m_texture_sampler;

    const rhi::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) } // NOSONAR
    };
};

} // namespace Methane::Tutorials
