/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

#include <Methane/Samples/AppSettings.hpp>

namespace Methane::Tutorials
{

TextRenderApp::TextRenderApp()
    : GraphicsApp(
        Samples::GetAppSettings("Methane Text Rendering", false /* animations */, true /* logo */, true /* hud ui */, false /* depth */),
        "Methane tutorial of text rendering")
{
}

TextRenderApp::~TextRenderApp()
{
    // Wait for GPU rendering is completed to release resources
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::RenderComplete);

    // Clear the font library to release all atlas textures
    gfx::Font::Library::Get().Clear();
}

void TextRenderApp::Init()
{
    GraphicsApp::Init();

    const gfx::RenderContext::Settings& context_settings = m_sp_context->GetSettings();

    // Add font to library
    m_sp_font = gfx::Font::Library::Get().AddFont(
        Data::FontProvider::Get(),
        gfx::Font::Settings{
            "Default", "Fonts/Roboto/Roboto-Regular.ttf", 24, m_sp_context->GetFontResolutionDPI(),
            gfx::Font::GetAnsiCharacters() + "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя"
        }
    ).GetPtr();

    // Create font atlas texture and badge to displaying it on screen
    const Ptr<gfx::Texture>& sp_font_atlas_texture = m_sp_font->GetAtlasTexturePtr(*m_sp_context);
    m_sp_font_atlas_badge = std::make_shared<gfx::Badge>(*m_sp_context, sp_font_atlas_texture,
        gfx::Badge::Settings
        {
            static_cast<const gfx::FrameSize&>(sp_font_atlas_texture->GetSettings().dimensions),
            gfx::Badge::FrameCorner::BottomRight,
            16u,
            1.f,
            gfx::Badge::TextureMode::RFloatToAlpha
        }
    );

    // Create text rendering primitive bound to the font object

    m_sp_text = std::make_shared<gfx::Text>(*m_sp_context, *m_sp_font,
        gfx::Text::Settings
        {
            "Label",
            "Wow... The quick brown fox jumps over the lazy dog!\n"
            "Cъешь ещё этих мягких французских булок, да выпей чаю.",
            gfx::FrameRect{ { 50, 100 }, { context_settings.frame_size.width * 2 / 3, context_settings.frame_size.width / 4 } },
            gfx::Color4f(1.f, 1.f, 1.f, 1.f)
        }
    );

    // Create per-frame command lists
    for(TextRenderFrame& frame : m_frames)
    {
        frame.sp_render_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_render_cmd_list->SetName(IndexedName("Text Rendering", frame.index));

        frame.sp_execute_cmd_lists = gfx::CommandListSet::Create({ *frame.sp_render_cmd_list });
    }

    // Complete initialization of render context
    m_sp_context->CompleteInitialization();
}

bool TextRenderApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    m_sp_font_atlas_badge->Resize(frame_size);
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

    m_sp_text->Draw(*frame.sp_render_cmd_list);
    m_sp_font_atlas_badge->Draw(*frame.sp_render_cmd_list);

    RenderOverlay(*frame.sp_render_cmd_list);

    // Commit command list with present flag
    frame.sp_render_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_lists);
    m_sp_context->Present();

    return true;
}

void TextRenderApp::OnContextReleased()
{
    gfx::Font::Library::Get().Clear();

    m_sp_font.reset();
    m_sp_text.reset();
    m_sp_font_atlas_badge.reset();

    GraphicsApp::OnContextReleased();
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TextRenderApp().Run({ argc, argv });
}
