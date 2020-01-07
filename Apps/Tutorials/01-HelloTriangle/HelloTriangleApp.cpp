/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

#include <cassert>

namespace Methane::Tutorials
{

static const gfx::Shader::EntryFunction g_vs_main      = { "Triangle", "TriangleVS" };
static const gfx::Shader::EntryFunction g_ps_main      = { "Triangle", "TrianglePS" };
static const GraphicsApp::Settings      g_app_settings = // Application settings:
{                                                   // ====================
    {                                               // app:
        "Methane Hello Triangle",                   // - name
        0.8, 0.8,                                   // - width, height
    },                                              //
    {                                               // context:
        gfx::FrameSize(),                           // - frame_size placeholder: actual size is set in InitContext
        gfx::PixelFormat::BGRA8Unorm,               // - color_format
        gfx::PixelFormat::Unknown,                  // - depth_stencil_format
        gfx::Color4f(0.0f, 0.2f, 0.4f, 1.0f),       // - clear_color
        { /* no depth-stencil clearing */ },        // - clear_depth_stencil
        3,                                          // - frame_buffers_count
        true,                                       // - vsync_enabled
    },                                              //
    true                                            // show_hud_in_window_title
};

HelloTriangleApp::HelloTriangleApp()
    : GraphicsApp(g_app_settings, gfx::RenderPass::Access::ShaderResources | gfx::RenderPass::Access::Samplers)
    , m_triangle_vertices({{
        { { 0.0f,   0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f,  -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } },
    }})
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

    assert(m_sp_context);

    // Create Methane logo badge
    m_sp_logo_badge = std::make_shared<gfx::LogoBadge>(*m_sp_context);

    // Create triangle shading program
    m_sp_program = gfx::Program::Create(*m_sp_context, {
        { // shaders
            gfx::Shader::CreateVertex(*m_sp_context, { Data::ShaderProvider::Get(), g_vs_main }),
            gfx::Shader::CreatePixel( *m_sp_context, { Data::ShaderProvider::Get(), g_ps_main }),
        },
        { // input_buffer_layouts
            { // single vertex buffer layout with interleaved data
                { // input arguments mapping to semantic names
                    { "input_position", "POSITION" },
                    { "input_color",    "COLOR"    },
                }
            }
        },
        { // constant_argument_names
        },
        { // addressable_argument_names
        },
        { // render_target_pixel_formats
            GetInitialContextSettings().color_format
        }
    });
    m_sp_program->SetName("Colored Vertices");

    // Create per-frame command lists
    for(HelloTriangleFrame& frame : m_frames)
    {
        frame.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Triangle Rendering", frame.index));
    }

    // Create vertex buffer with triangle data
    const Data::Size vertex_size      = static_cast<Data::Size>(sizeof(Vertex));
    const Data::Size vertex_data_size = static_cast<Data::Size>(sizeof(m_triangle_vertices));
    
    m_sp_vertex_buffer = gfx::Buffer::CreateVertexBuffer(*m_sp_context, vertex_data_size, vertex_size);
    m_sp_vertex_buffer->SetName("Triangle Vertex Buffer");
    m_sp_vertex_buffer->SetData({ { reinterpret_cast<Data::ConstRawPtr>(m_triangle_vertices.data()), vertex_data_size } });

    // Create render state
    m_sp_state = gfx::RenderState::Create(*m_sp_context, {
        m_sp_program,
        { gfx::GetFrameViewport(GetInitialContextSettings().frame_size) },
        { gfx::GetFrameScissorRect(GetInitialContextSettings().frame_size) },
    });
    m_sp_state->SetName("Frame Render Pipeline State");

    // Complete initialization of render context
    m_sp_context->CompleteInitialization();
}

bool HelloTriangleApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    // Update viewports and scissor rects state
    assert(m_sp_state);
    m_sp_state->SetViewports(    { gfx::GetFrameViewport(frame_size)    } );
    m_sp_state->SetScissorRects( { gfx::GetFrameScissorRect(frame_size) } );
    m_sp_logo_badge->Resize(frame_size);

    return true;
}

bool HelloTriangleApp::Render()
{
    // Render only when context is ready
    assert(!!m_sp_context);
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    HelloTriangleFrame& frame = GetCurrentFrame();

    assert(!!frame.sp_cmd_list);
    assert(!!m_sp_vertex_buffer);
    assert(!!m_sp_state);

    // Issue commands for triangle rendering
    frame.sp_cmd_list->Reset(m_sp_state, "Cube redering");
    frame.sp_cmd_list->SetVertexBuffers({ *m_sp_vertex_buffer });
    frame.sp_cmd_list->Draw(gfx::RenderCommandList::Primitive::Triangle, static_cast<uint32_t>(m_triangle_vertices.size()));

    // Draw Methane logo badge
    assert(!!m_sp_logo_badge);
    m_sp_logo_badge->Draw(*frame.sp_cmd_list);

    // Commit command list with present flag
    frame.sp_cmd_list->Commit(true);

    // Execute command list on render queue and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
    m_sp_context->Present();

    return true;
}

void HelloTriangleApp::OnContextReleased()
{
    m_sp_vertex_buffer.reset();
    m_sp_state.reset();
    m_sp_program.reset();

    GraphicsApp::OnContextReleased();
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::HelloTriangleApp().Run({ argc, argv });
}
