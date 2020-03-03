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

namespace Methane::Tutorials
{

static const GraphicsApp::AllSettings g_app_settings =  // Application settings:
{                                                       // ====================
    {                                                   // platform_apps:
        "Methane Hello Triangle",                       // - name
        0.8, 0.8,                                       // - width, height
    },                                                  //
    {                                                   // graphics_app:
        gfx::RenderPass::Access::ShaderResources |      // - screen_pass_access
        gfx::RenderPass::Access::Samplers,              //
        true,                                           // - animations_enabled
        true,                                           // - show_hud_in_window_title
        true,                                           // - show_logo_badge
        0                                               // - default_device_index
    },                                                  //
    {                                                   // render_context:
        gfx::FrameSize(),                               // - frame_size placeholder: actual size is set in InitContext
        gfx::PixelFormat::BGRA8Unorm,                   // - color_format
        gfx::PixelFormat::Unknown,                      // - depth_stencil_format
        gfx::Color4f(0.0f, 0.2f, 0.4f, 1.0f),           // - clear_color
        { /* no depth-stencil clearing */ },            // - clear_depth_stencil
        3,                                              // - frame_buffers_count
        false,                                          // - vsync_enabled
    }
};

HelloTriangleApp::HelloTriangleApp()
    : GraphicsApp(g_app_settings, "Methane tutorial of simple triangle rendering")
{
}

HelloTriangleApp::~HelloTriangleApp()
{
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void HelloTriangleApp::Init()
{
    GraphicsApp::Init();

    struct Vertex
    {
        gfx::Vector3f position;
        gfx::Vector3f color;
    };

    const std::array<Vertex, 3> triangle_vertices = {{
        { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    }};

    // Create vertex buffer with triangle data
    const Data::Size vertex_size      = static_cast<Data::Size>(sizeof(Vertex));
    const Data::Size vertex_data_size = static_cast<Data::Size>(sizeof(triangle_vertices));

    m_sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(*m_sp_context, vertex_data_size, vertex_size);
    m_sp_vertex_buffer->SetName("Triangle Vertex Buffer");
    m_sp_vertex_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(triangle_vertices.data()), vertex_data_size } });

    // Create render state
    m_sp_state = gfx::RenderState::Create(*m_sp_context,
        gfx::RenderState::Settings
        {
            gfx::Program::Create(*m_sp_context,
                gfx::Program::Settings
                {
                    gfx::Program::Shaders
                    {
                        gfx::Shader::CreateVertex(*m_sp_context, { Data::ShaderProvider::Get(), { "Triangle", "TriangleVS" } }),
                        gfx::Shader::CreatePixel( *m_sp_context, { Data::ShaderProvider::Get(), { "Triangle", "TrianglePS" } }),
                    },
                    gfx::Program::InputBufferLayouts
                    {
                        gfx::Program::InputBufferLayout
                        {
                            gfx::Program::InputBufferLayout::ArgumentSemantics { "POSITION" , "COLOR" }
                        }
                    },
                    gfx::Program::ArgumentDescriptions { },
                    gfx::PixelFormats { m_sp_context->GetSettings().color_format }
                }
            ),
            gfx::Viewports
            {
                gfx::GetFrameViewport(GetInitialContextSettings().frame_size)
            },
            gfx::ScissorRects
            {
                gfx::GetFrameScissorRect(GetInitialContextSettings().frame_size)
            },
        }
    );
    m_sp_state->GetSettings().sp_program->SetName("Colored Triangle Shading");
    m_sp_state->SetName("Triangle Pipeline State");

    // Create per-frame command lists
    for(HelloTriangleFrame& frame : m_frames)
    {
        frame.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Triangle Rendering", frame.index));
    }

    // Complete initialization of render context
    m_sp_context->CompleteInitialization();
}

bool HelloTriangleApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    // Update viewports and scissor rects state
    m_sp_state->SetViewports(    { gfx::GetFrameViewport(frame_size)    } );
    m_sp_state->SetScissorRects( { gfx::GetFrameScissorRect(frame_size) } );

    return true;
}

bool HelloTriangleApp::Render()
{
    // Render only when context is ready
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    HelloTriangleFrame& frame = GetCurrentFrame();

    // Issue commands for triangle rendering
    frame.sp_cmd_list->Reset(m_sp_state, "Triangle Rendering");
    frame.sp_cmd_list->SetVertexBuffers({ *m_sp_vertex_buffer });
    frame.sp_cmd_list->Draw(gfx::RenderCommandList::Primitive::Triangle, 3u);

    RenderOverlay(*frame.sp_cmd_list);

    // Commit command list with present flag
    frame.sp_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
    m_sp_context->Present();

    return true;
}

void HelloTriangleApp::OnContextReleased()
{
    m_sp_vertex_buffer.reset();
    m_sp_state.reset();

    GraphicsApp::OnContextReleased();
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::HelloTriangleApp().Run({ argc, argv });
}
