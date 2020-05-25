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

struct FontSettings
{
    std::string  name;
    std::string  path;
    uint32_t     size;
    gfx::Color3f color;
};

constexpr    int32_t                             g_margin_size       = 32;
static const FontSettings                        g_primary_font      { "Primary",   "Fonts/Roboto/Roboto-Regular.ttf",     24u, { 1.f,  1.f, 0.5f } };
static const FontSettings                        g_secondary_font    { "Secondary", "Fonts/Playball/Playball-Regular.ttf", 16u, { 0.5f, 1.f, 0.5f } };
static const gfx::Color3f                        g_misc_font_color   { 1.f, 1.f, 1.f };
static const std::map<std::string, gfx::Color3f> g_font_color_by_name{
    { g_primary_font.name,   g_primary_font.color   },
    { g_secondary_font.name, g_secondary_font.color }
};

static const std::string g_cyrilyc_chars = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя";
static const std::string g_pangram_eng   = "The quick brown fox jumps over the lazy dog!";
static const std::string g_pangram_rus   = "Cъешь ещё этих мягких французских булок, да выпей чаю.";
static const std::string g_hitchhikers_guide = "A towel is about the most massively useful thing an interstellar hitchhiker can have. " \
    "Partly it has great practical value. You can wrap it around you for warmth as you bound across the cold moons of Jaglan Beta; " \
    "you can lie on it on the brilliant marble-sanded beaches of Santraginus V, inhaling the heady sea vapors; " \
    "you can sleep under it beneath the stars which shine so redly on the desert world of Kakrafoon; " \
    "use it to sail a miniraft down the slow heavy River Moth; " \
    "wet it for use in hand-to-hand-combat; " \
    "wrap it round your head to ward off noxious fumes or avoid the gaze of the Ravenous Bugblatter Beast of Traal " \
    "(such a mind-boggingly stupid animal, it assumes that if you can't see it, it can't see you); " \
    "you can wave your towel in emergencies as a distress signal, and of course dry yourself off with it if it still seems to be clean enough.";

TextRenderApp::TextRenderApp()
    : GraphicsApp(
        Samples::GetAppSettings("Methane Text Rendering", false /* animations */, true /* logo */, true /* hud ui */, false /* depth */),
        "Methane tutorial of text rendering")
{
    GetHeadsUpDisplaySettings().position = gfx::Point2i(g_margin_size, g_margin_size);
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

    // Add fonts to library
    m_sp_primary_font = gfx::Font::Library::Get().AddFont(
        Data::FontProvider::Get(),
        gfx::Font::Settings{
            g_primary_font.name, g_primary_font.path, 24, m_sp_context->GetFontResolutionDPI(),
            gfx::Font::GetAnsiCharacters() + g_cyrilyc_chars
        }
    ).GetPtr();

    m_sp_secondary_font = gfx::Font::Library::Get().AddFont(
        Data::FontProvider::Get(),
        gfx::Font::Settings{
            g_secondary_font.name, g_secondary_font.path, 24, m_sp_context->GetFontResolutionDPI(),
            gfx::Font::GetAnsiCharacters()
        }
    ).GetPtr();

    // Create text rendering primitive bound to the font object
    m_sp_primary_text = std::make_shared<gfx::Text>(*m_sp_context, *m_sp_primary_font,
        gfx::Text::Settings
        {
            "Panagrams",
            g_pangram_eng + "\n" + g_pangram_rus,
            gfx::FrameRect{ { g_margin_size, 100 }, { context_settings.frame_size.width / 3, context_settings.frame_size.height / 8 } },
            gfx::Color4f(g_primary_font.color, 1.f)
        }
    );

    m_sp_secondary_text = std::make_shared<gfx::Text>(*m_sp_context, *m_sp_secondary_font,
        gfx::Text::Settings
        {
            "Hitchhikers Guide",
            g_hitchhikers_guide,
            gfx::FrameRect{ { g_margin_size, 200 }, { context_settings.frame_size.width / 3, context_settings.frame_size.height / 6 } },
            gfx::Color4f(g_secondary_font.color, 1.f)
        }
    );

    InitFontAtlasBadges();

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

void TextRenderApp::InitFontAtlasBadges()
{
    const Refs<gfx::Font> font_refs = gfx::Font::Library::Get().GetFonts();
    gfx::Context& context = *m_sp_context;

    // Remove obsolete font atlas badges
    for(auto badge_it = m_sp_font_atlas_badges.begin(); badge_it != m_sp_font_atlas_badges.end();)
    {
        const Ptr<gfx::Badge>& sp_font_atlas_badge = *badge_it;
        const auto font_ref_it = std::find_if(font_refs.begin(), font_refs.end(),
            [&sp_font_atlas_badge, &context](const Ref<gfx::Font>& font_ref)
            {
               return std::addressof(sp_font_atlas_badge->GetTexture()) == &font_ref.get().GetAtlasTexture(context);
            }
        );
        if (font_ref_it == font_refs.end())
        {
            badge_it = m_sp_font_atlas_badges.erase(badge_it);
        }
        else
        {
            ++badge_it;
        }
    }

    // Add new font atlas badges
    for(const Ref<gfx::Font>& font_ref : font_refs)
    {
        const Ptr<gfx::Texture>& sp_font_atlas_texture = font_ref.get().GetAtlasTexturePtr(context);
        const auto sp_font_atlas_it = std::find_if(m_sp_font_atlas_badges.begin(), m_sp_font_atlas_badges.end(),
            [&sp_font_atlas_texture](const Ptr<gfx::Badge>& sp_font_atlas_badge)
            {
                return std::addressof(sp_font_atlas_badge->GetTexture()) == sp_font_atlas_texture.get();
            }
        );

        if (sp_font_atlas_it != m_sp_font_atlas_badges.end())
            continue;

        const auto font_color_by_name_it = g_font_color_by_name.find(font_ref.get().GetSettings().name);
        const gfx::Color3f& font_color = font_color_by_name_it != g_font_color_by_name.end()
                                       ? font_color_by_name_it->second : g_misc_font_color;

        m_sp_font_atlas_badges.emplace_back(
            std::make_shared<gfx::Badge>(
                *m_sp_context, sp_font_atlas_texture,
                gfx::Badge::Settings
                {
                    static_cast<const gfx::FrameSize&>(sp_font_atlas_texture->GetSettings().dimensions),
                    gfx::Badge::FrameCorner::BottomLeft,
                    gfx::Point2u(16u, 16u),
                    gfx::Color4f(font_color, 0.5f),
                    gfx::Badge::TextureMode::RFloatToAlpha
                }
            )
        );
    }

    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

void TextRenderApp::LayoutFontAtlasBadges(const gfx::FrameSize& frame_size)
{
    // Sort atlas badges by size so that largest are displayed first
    std::sort(m_sp_font_atlas_badges.begin(), m_sp_font_atlas_badges.end(),
              [](const Ptr<gfx::Badge>& sp_left, const Ptr<gfx::Badge>& sp_right)
              {
                  return sp_left->GetSettings().frame_rect.size.GetPixelsCount() >
                         sp_right->GetSettings().frame_rect.size.GetPixelsCount();
              }
    );

    const float scale_factor = GetRenderContext().GetContentScalingFactor();
    gfx::Point2i badge_margins(g_margin_size, g_margin_size);
    badge_margins *= scale_factor;

    // Layout badges in a row one after another with a margin spacing
    for(const Ptr<gfx::Badge>& sp_badge_atlas : m_sp_font_atlas_badges)
    {
        assert(sp_badge_atlas);
        const gfx::FrameSize& atlas_size = static_cast<const gfx::FrameSize&>(sp_badge_atlas->GetTexture().GetSettings().dimensions);
        sp_badge_atlas->FrameResize(frame_size, atlas_size, badge_margins);
        badge_margins.SetX(badge_margins.GetX() + atlas_size.width + static_cast<int32_t>(scale_factor * g_margin_size));
    }
}

bool TextRenderApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    LayoutFontAtlasBadges(frame_size);
    return true;
}

bool TextRenderApp::Render()
{
    // Render only when context is ready
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Wait for previous frame rendering is completed and switch to next frame
    m_sp_context->WaitForGpu(gfx::Context::WaitFor::FramePresented);

    // Draw text blocks
    TextRenderFrame& frame = GetCurrentFrame();
    m_sp_primary_text->Draw(*frame.sp_render_cmd_list);
    m_sp_secondary_text->Draw(*frame.sp_render_cmd_list);

    // Draw font atlas badges
    for(const Ptr<gfx::Badge>& sp_badge_atlas : m_sp_font_atlas_badges)
    {
        sp_badge_atlas->Draw(*frame.sp_render_cmd_list);
    }

    RenderOverlay(*frame.sp_render_cmd_list);

    // Commit command list with present flag
    frame.sp_render_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    m_sp_context->GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_lists);
    m_sp_context->Present();

    return true;
}

void TextRenderApp::OnContextReleased(gfx::Context& context)
{
    gfx::Font::Library::Get().Clear();

    m_sp_primary_font.reset();
    m_sp_secondary_font.reset();
    m_sp_primary_text.reset();
    m_sp_secondary_text.reset();
    m_sp_font_atlas_badges.clear();

    GraphicsApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TextRenderApp().Run({ argc, argv });
}
