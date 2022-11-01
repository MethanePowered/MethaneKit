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

struct TexturedCubeFrame final : Graphics::AppFrame
{
    Ptr<gfx::IBuffer>           uniforms_buffer_ptr;
    Ptr<gfx::IProgramBindings>  program_bindings_ptr;
    Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
    Ptr<gfx::ICommandListSet>   execute_cmd_list_set_ptr;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<TexturedCubeFrame>;

class TexturedCubeApp final : public UserInterfaceApp // NOSONAR
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
    void OnContextReleased(gfx::IContext& context) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);

    const float             m_cube_scale = 15.F;
    const hlslpp::Constants m_shader_constants{
        { 1.F, 1.F, 0.74F, 1.F },  // - light_color
        700.F,                     // - light_power
        0.04F,                     // - light_ambient_factor
        30.F                       // - light_specular_factor
    };
    hlslpp::Uniforms       m_shader_uniforms { };
    gfx::Camera            m_camera;
    Ptr<gfx::IRenderState> m_render_state_ptr;
    Ptr<gfx::IBufferSet>   m_vertex_buffer_set_ptr;
    Ptr<gfx::IBuffer>      m_index_buffer_ptr;
    Ptr<gfx::IBuffer>      m_const_buffer_ptr;
    Ptr<gfx::ITexture>     m_cube_texture_ptr;
    Ptr<gfx::ISampler>     m_texture_sampler_ptr;

    const gfx::IResource::SubResources m_shader_uniforms_subresources{
        { reinterpret_cast<Data::ConstRawPtr>(&m_shader_uniforms), sizeof(hlslpp::Uniforms) } // NOSONAR
    };
};

} // namespace Methane::Tutorials
