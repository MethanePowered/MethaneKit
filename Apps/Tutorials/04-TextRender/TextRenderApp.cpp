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

FILE: TextRenderApp.cpp
Tutorial demonstrating text rendering with Methane graphics API

******************************************************************************/

#include "TextRenderApp.h"

namespace Methane::Tutorials
{

static const GraphicsApp::AllSettings g_app_settings =  // Application settings:
{                                                       // ====================
    {                                                   // platform_apps:
        "Methane Text Rendering",                       // - name
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

TextRenderApp::TextRenderApp()
    : GraphicsApp(g_app_settings, "Methane tutorial of text rendering")
{
}

TextRenderApp::~TextRenderApp()
{
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);
}

void TextRenderApp::Init()
{
    GraphicsApp::Init();

    m_sp_text = std::make_shared<gfx::Text>(*m_sp_context, gfx::Text::Settings{ "Label", gfx::FrameRect() });

    // Create per-frame command lists
    for(TextRenderFrame& frame : m_frames)
    {
        frame.sp_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_cmd_list->SetName(IndexedName("Text Rendering", frame.index));
    }

    // Complete initialization of render context
    m_sp_context->CompleteInitialization();
}

bool TextRenderApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    // ...
    return true;
}

bool TextRenderApp::Render()
{
    // Render only when context is ready
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);
    TextRenderFrame& frame = GetCurrentFrame();

    RenderOverlay(*frame.sp_cmd_list);

    // Commit command list with present flag
    frame.sp_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute({ *frame.sp_cmd_list });
    m_sp_context->Present();

    return true;
}

void TextRenderApp::OnContextReleased()
{
    m_sp_text.reset();

    GraphicsApp::OnContextReleased();
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TextRenderApp().Run({ argc, argv });
}
