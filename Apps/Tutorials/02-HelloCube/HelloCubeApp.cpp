/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: HelloCubeApp.cpp
Tutorial demonstrating colored cube rendering with Methane graphics API

******************************************************************************/

#include "HelloCubeApp.h"

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Data/TimeAnimation.h>

namespace Methane::Tutorials
{

HelloCubeApp::HelloCubeApp()
    : GraphicsApp(
        []() {
            Graphics::AppSettings settings = Samples::GetGraphicsAppSettings("Methane Hello Cube", Samples::g_default_app_options_color_only_and_anim);
            settings.graphics_app.SetScreenPassAccess(gfx::RenderPass::Access::None);
            return settings;
        }())
    , m_cube_mesh(CubeVertex::layout)
    , m_proj_vertices(m_cube_mesh.GetVertices())
    , m_model_matrix(hlslpp::float4x4::scale(15.F))
{
    m_camera.ResetOrientation({ { 13.0F, 13.0F, 13.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } });

    // Setup animations
    GetAnimations().emplace_back(std::make_shared<Data::TimeAnimation>(std::bind(&HelloCubeApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
}

HelloCubeApp::~HelloCubeApp()
{
    // Wait for GPU rendering is completed to release resources
    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void HelloCubeApp::Init()
{
    GraphicsApp::Init();

    const gfx::RenderContext::Settings& context_settings = GetRenderContext().GetSettings();
    m_camera.Resize({
        static_cast<float>(context_settings.frame_size.GetWidth()),
        static_cast<float>(context_settings.frame_size.GetHeight())
    });

    // Create vertex buffer for cube mesh
    Ptr<gfx::Buffer> vertex_buffer_ptr = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), m_cube_mesh.GetVertexDataSize(), m_cube_mesh.GetVertexSize());
    vertex_buffer_ptr->SetName("Cube Vertex Buffer");
    m_vertex_buffer_set_ptr = gfx::BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    // Create index buffer for cube mesh
    m_index_buffer_ptr = gfx::Buffer::CreateIndexBuffer(GetRenderContext(), m_cube_mesh.GetIndexDataSize(), gfx::GetIndexFormat(m_cube_mesh.GetIndex(0)));
    m_index_buffer_ptr->SetName("Cube Index Buffer");
    m_index_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_cube_mesh.GetIndices().data()), m_cube_mesh.GetIndexDataSize() } });

    // Create render state with program
    gfx::RenderState::Settings state_settings;
    state_settings.program_ptr = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "HelloCube", "CubeVS" } }),
                gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "HelloCube", "CubePS" } }),
            },
            gfx::Program::InputBufferLayouts
            {
                gfx::Program::InputBufferLayout
                {
                    gfx::Program::InputBufferLayout::ArgumentSemantics { "POSITION" , "COLOR" }
                }
            },
            gfx::Program::ArgumentAccessors
            {
                { { gfx::Shader::Type::All, "g_uniforms"  }, gfx::Program::ArgumentAccessor::Type::FrameConstant },
            },
            GetScreenPassPattern().GetAttachmentFormats()
        }
    );
    state_settings.render_pattern_ptr = GetScreenPassPatternPtr();
    state_settings.program_ptr->SetName("Colored Cube Shading");
    m_render_state_ptr = gfx::RenderState::Create(GetRenderContext(), state_settings);
    m_render_state_ptr->SetName("Colored Cube Pipeline State");

    // Create per-frame command lists
    for(HelloCubeFrame& frame : GetFrames())
    {
        // Create command list for rendering
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr });
    }

    GraphicsApp::CompleteInitialization();
}

bool HelloCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize({
        static_cast<float>(frame_size.GetWidth()),
        static_cast<float>(frame_size.GetHeight())
    });

    return true;
}

bool HelloCubeApp::Animate(double, double delta_seconds)
{
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.F / 8.F));
    return true;
}

bool HelloCubeApp::Update()
{
    if (!GraphicsApp::Update())
        return false;

    // Update vertex buffer with camera Model-View-Projection matrix applied on CPU
    const hlslpp::float4x4 mvp_matrix = hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix());
    for(size_t vertex_index = 0; vertex_index < m_proj_vertices.size(); ++vertex_index)
    {
        const hlslpp::float4 orig_position_vec(m_cube_mesh.GetVertices()[vertex_index].position.AsHlsl(), 1.F);
        const hlslpp::float4 proj_position_vec = hlslpp::mul(orig_position_vec, mvp_matrix);
        m_proj_vertices[vertex_index].position = gfx::Mesh::Position(proj_position_vec.xyz / proj_position_vec.w);
    }

    // Update vertex buffer with vertices in camera's projection view
    (*m_vertex_buffer_set_ptr)[0].SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_proj_vertices.data()), m_cube_mesh.GetVertexDataSize() } },
                                          &GetRenderContext().GetRenderCommandKit().GetQueue());
    return true;
}

bool HelloCubeApp::Render()
{
    if (!GraphicsApp::Render())
        return false;

    // Issue commands for cube rendering
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    const HelloCubeFrame& frame = GetCurrentFrame();
    frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
    frame.render_cmd_list_ptr->SetViewState(GetViewState());
    frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
    frame.render_cmd_list_ptr->SetIndexBuffer(*m_index_buffer_ptr);
    frame.render_cmd_list_ptr->DrawIndexed(gfx::RenderCommandList::Primitive::Triangle);
    frame.render_cmd_list_ptr->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(*frame.execute_cmd_list_set_ptr);
    GetRenderContext().Present();

    return true;
}

void HelloCubeApp::OnContextReleased(gfx::Context& context)
{
    m_index_buffer_ptr.reset();
    m_vertex_buffer_set_ptr.reset();
    m_render_state_ptr.reset();

    GraphicsApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::HelloCubeApp().Run({ argc, argv });
}
