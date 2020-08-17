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

FILE: HelloTriangleApp.cpp
Tutorial demonstrating triangle rendering with Methane graphics API

******************************************************************************/

#include "HelloTriangleApp.h"

#include <Methane/Samples/AppSettings.hpp>

namespace Methane::Tutorials
{

HelloTriangleApp::HelloTriangleApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Hello Triangle", false /* animations */, false /* depth */), {},
        "Methane tutorial of simple triangle rendering")
{
}

HelloTriangleApp::~HelloTriangleApp()
{
    // Wait for GPU rendering is completed to release resources
    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void HelloTriangleApp::Init()
{
    UserInterfaceApp::Init();

    struct Vertex
    {
        gfx::Vector3f position;
        gfx::Vector3f color;
    };

    const std::array<Vertex, 3> triangle_vertices{ {
        { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    } };

    // Create vertex buffer with triangle data
    const Data::Size vertex_size      = static_cast<Data::Size>(sizeof(Vertex));
    const Data::Size vertex_data_size = static_cast<Data::Size>(sizeof(triangle_vertices));

    Ptr<gfx::Buffer> sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), vertex_data_size, vertex_size);
    sp_vertex_buffer->SetName("Triangle Vertex Buffer");
    sp_vertex_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(triangle_vertices.data()), vertex_data_size } });
    m_sp_vertex_buffer_set = gfx::BufferSet::CreateVertexBuffers({ *sp_vertex_buffer });

    // Create render state
    m_sp_render_state = gfx::RenderState::Create(GetRenderContext(),
        gfx::RenderState::Settings
        {
            gfx::Program::Create(GetRenderContext(),
                gfx::Program::Settings
                {
                    gfx::Program::Shaders
                    {
                        gfx::Shader::CreateVertex(GetRenderContext(), { Data::ShaderProvider::Get(), { "Triangle", "TriangleVS" } }),
                        gfx::Shader::CreatePixel( GetRenderContext(), { Data::ShaderProvider::Get(), { "Triangle", "TrianglePS" } }),
                    },
                    gfx::Program::InputBufferLayouts
                    {
                        gfx::Program::InputBufferLayout
                        {
                            gfx::Program::InputBufferLayout::ArgumentSemantics { "POSITION" , "COLOR" }
                        }
                    },
                    gfx::Program::ArgumentDescriptions { },
                    gfx::PixelFormats { GetRenderContext().GetSettings().color_format }
                }
            )
        }
    );
    m_sp_render_state->GetSettings().sp_program->SetName("Colored Triangle Shading");
    m_sp_render_state->SetName("Triangle Pipeline State");

    // Create per-frame command lists
    for(HelloTriangleFrame& frame : GetFrames())
    {
        frame.sp_render_cmd_list = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_render_cmd_list->SetName(IndexedName("Triangle Rendering", frame.index));
        frame.sp_execute_cmd_list_set = gfx::CommandListSet::Create({ *frame.sp_render_cmd_list });
    }

    UserInterfaceApp::CompleteInitialization();
}

bool HelloTriangleApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    // Issue commands for triangle rendering
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Triangle Rendering");
    HelloTriangleFrame& frame = GetCurrentFrame();
    frame.sp_render_cmd_list->Reset(m_sp_render_state, s_debug_group.get());
    frame.sp_render_cmd_list->SetViewState(GetViewState());
    frame.sp_render_cmd_list->SetVertexBuffers(*m_sp_vertex_buffer_set);
    frame.sp_render_cmd_list->Draw(gfx::RenderCommandList::Primitive::Triangle, 3u);

    RenderOverlay(*frame.sp_render_cmd_list);

    // Commit command list with present flag
    frame.sp_render_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}

void HelloTriangleApp::OnContextReleased(gfx::Context& context)
{
    m_sp_vertex_buffer_set.reset();
    m_sp_render_state.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::HelloTriangleApp().Run({ argc, argv });
}
