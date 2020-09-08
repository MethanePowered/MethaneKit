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

    Ptr<gfx::Buffer> vertex_buffer_ptr = gfx::Buffer::CreateVertexBuffer(GetRenderContext(), vertex_data_size, vertex_size);
    vertex_buffer_ptr->SetName("Triangle Vertex Buffer");
    vertex_buffer_ptr->SetData({ { reinterpret_cast<Data::ConstRawPtr>(triangle_vertices.data()), vertex_data_size } });
    m_vertex_buffer_set_ptr = gfx::BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    // Create render state
    m_render_state_ptr = gfx::RenderState::Create(GetRenderContext(),
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
    m_render_state_ptr->GetSettings().program_ptr->SetName("Colored Triangle Shading");
    m_render_state_ptr->SetName("Triangle Pipeline State");

    // Create per-frame command lists
    for(HelloTriangleFrame& frame : GetFrames())
    {
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Triangle Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr });
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
    frame.render_cmd_list_ptr->Reset(m_render_state_ptr, s_debug_group.get());
    frame.render_cmd_list_ptr->SetViewState(GetViewState());
    frame.render_cmd_list_ptr->SetVertexBuffers(*m_vertex_buffer_set_ptr);
    frame.render_cmd_list_ptr->Draw(gfx::RenderCommandList::Primitive::Triangle, 3u);

    RenderOverlay(*frame.render_cmd_list_ptr);

    // Commit command list with present flag
    frame.render_cmd_list_ptr->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.execute_cmd_list_set_ptr);
    GetRenderContext().Present();

    return true;
}

void HelloTriangleApp::OnContextReleased(gfx::Context& context)
{
    m_vertex_buffer_set_ptr.reset();
    m_render_state_ptr.reset();

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::HelloTriangleApp().Run({ argc, argv });
}
