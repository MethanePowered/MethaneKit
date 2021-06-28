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
#include <Methane/Graphics/Mesh/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.h>

namespace Methane::Tutorials
{

struct CubeVertex
{
    gfx::Mesh::Position position;
    gfx::Mesh::Color    color;

    inline static const gfx::Mesh::VertexLayout layout{
        gfx::Mesh::VertexField::Position,
        gfx::Mesh::VertexField::Color
    };
};

HelloCubeApp::HelloCubeApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Hello Cube", Samples::AppOptions::Animations), {},
        "Methane tutorial of colored cube rendering")
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
    UserInterfaceApp::Init();

    const gfx::RenderContext::Settings& context_settings = GetRenderContext().GetSettings();
    m_camera.Resize({
        static_cast<float>(context_settings.frame_size.GetWidth()),
        static_cast<float>(context_settings.frame_size.GetHeight())
    });

    const gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);

    // Create vertex buffer for cube mesh
    const Data::Size vertex_data_size = cube_mesh.GetVertexDataSize();
    const Data::Size vertex_size      = cube_mesh.GetVertexSize();
    Ptr<gfx::Buffer> vertex_buffer_ptr = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), vertex_data_size, vertex_size);
    vertex_buffer_ptr->SetName("Cube Vertex Buffer");
    vertex_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetVertices().data()), vertex_data_size } });
    m_vertex_buffer_set_ptr = gfx::BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    // Create index buffer for cube mesh
    const Data::Size index_data_size = cube_mesh.GetIndexDataSize();
    m_index_buffer_ptr = gfx::Buffer::CreateIndexBuffer(GetRenderContext(), index_data_size, gfx::GetIndexFormat(cube_mesh.GetIndex(0)));
    m_index_buffer_ptr->SetName("Cube Index Buffer");
    m_index_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(cube_mesh.GetIndices().data()), index_data_size } });

    // Create render state with program
    gfx::RenderState::Settings state_settings;
    state_settings.program_ptr = gfx::Program::Create(GetRenderContext(),
        gfx::Program::Settings
        {
            gfx::Program::Shaders
            {
                gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "ColoredCube", "CubeVS" } }),
                gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "ColoredCube", "CubePS" } }),
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
    const Data::Size uniforms_data_size = gfx::Buffer::GetAlignedBufferSize(static_cast<Data::Size>(sizeof(m_shader_uniforms)));
    for(HelloCubeFrame& frame : GetFrames())
    {
        // Create uniforms buffer with volatile parameters for frame rendering
        frame.uniforms_buffer_ptr = gfx::Buffer::CreateVolatileBuffer(GetRenderContext(), uniforms_data_size);
        frame.uniforms_buffer_ptr->SetName(IndexedName("Uniforms Buffer", frame.index));

        // Configure program resource bindings
        frame.program_bindings_ptr = gfx::ProgramBindings::Create(state_settings.program_ptr, {
            { { gfx::Shader::Type::All, "g_uniforms" }, { { *frame.uniforms_buffer_ptr } } },
        }, frame.index);

        // Create command list for rendering
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandKit().GetQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Cube Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr });
    }

    UserInterfaceApp::CompleteInitialization();
}

bool HelloCubeApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
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
    if (!UserInterfaceApp::Update())
        return false;

    // Update Model-View-Projection matrix uniform
    m_shader_uniforms.mvp_matrix = hlslpp::transpose(hlslpp::mul(m_model_matrix, m_camera.GetViewProjMatrix()));

    return true;
}

bool HelloCubeApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Update uniforms buffer related to current frame
    const HelloCubeFrame& frame = GetCurrentFrame();
    frame.uniforms_buffer_ptr->SetData(m_shader_uniforms_subresources);

    // Issue commands for cube rendering
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Cube Rendering");
    frame.render_cmd_list_ptr->ResetWithState(*m_render_state_ptr, s_debug_group.get());
    frame.render_cmd_list_ptr->SetViewState(GetViewState());
    frame.render_cmd_list_ptr->SetProgramBindings(*frame.program_bindings_ptr);
    frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
    frame.render_cmd_list_ptr->SetIndexBuffer(*m_index_buffer_ptr);
    frame.render_cmd_list_ptr->DrawIndexed(gfx::RenderCommandList::Primitive::Triangle);

    RenderOverlay(*frame.render_cmd_list_ptr);

    // Commit command list with present flag
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

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::HelloCubeApp().Run({ argc, argv });
}
