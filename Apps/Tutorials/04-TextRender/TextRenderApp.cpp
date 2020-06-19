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
#include <Methane/Data/TimeAnimation.h>

#include <array>

namespace Methane::Tutorials
{

struct FontSettings
{
    std::string  name;
    std::string  path;
    uint32_t     size;
    gfx::Color3f color;
};

constexpr int32_t g_margin_size_in_dots      = 32;
constexpr int32_t g_top_text_pos_in_dots     = 100;
constexpr double  g_text_update_interval_sec = 0.03;
constexpr size_t  g_text_blocks_count        = 2;

static const std::array<FontSettings, g_text_blocks_count> g_font_settings { {
    { "European",     "Fonts/Roboto/Roboto-Regular.ttf",                 20u, { 1.f,  1.f,  0.5f } },
//  { "Japanese",     "Fonts/SawarabiMincho/SawarabiMincho-Regular.ttf", 20u, { 1.f,  0.3f, 0.1f } },
//  { "Arabic",       "Fonts/Amiri/Amiri-Regular.ttf",                   20u, { 1.f,  0.3f, 0.8f } },
    { "Calligraphic", "Fonts/Playball/Playball-Regular.ttf",             20u, { 0.5f, 1.f,  0.5f } }
} };

static const gfx::Color3f g_misc_font_color { 1.f, 1.f, 1.f };
static const std::map<std::string, gfx::Color3f>    g_font_color_by_name   {
    { g_font_settings[0].name, g_font_settings[0].color },
    { g_font_settings[1].name, g_font_settings[1].color },
//  { g_font_settings[2].name, g_font_settings[2].color },
//  { g_font_settings[3].name, g_font_settings[3].color }
};

// Pangrams from http://clagnut.com/blog/2380/
static const std::array<std::string, g_text_blocks_count> g_text_blocks = { {

    // 0: european pangrams
    "The quick brown fox jumps over the lazy dog!\n" \
    "Съешь ещё этих мягких французских булок, да выпей чаю.\n" \
    "Ο καλύμνιος σφουγγαράς ψιθύρισε πως θα βουτήξει χωρίς να διστάζει.\n" \
    "Pijamalı hasta, yağız şoföre çabucak güvendi.",

    // 1: japanese pangram
    //"いろはにほへと ちりぬるを わかよたれそ つねならむ うゐのおくやま けふこえて あさきゆめみし ゑひもせす",

    // 2: arabian pangram
    //"نص حكيم له سر قاطع وذو شأن عظيم مكتوب على ثوب أخضر ومغلف بجلد أزرق",

    // 3: hitchhicker's guide quote
    "A towel is about the most massively useful thing an interstellar hitchhiker can have. " \
    "Partly it has great practical value. You can wrap it around you for warmth as you bound across the cold moons of Jaglan Beta; " \
    "you can lie on it on the brilliant marble-sanded beaches of Santraginus V, inhaling the heady sea vapors; " \
    "you can sleep under it beneath the stars which shine so redly on the desert world of Kakrafoon; " \
    "use it to sail a miniraft down the slow heavy River Moth; " \
    "wet it for use in hand-to-hand-combat; " \
    "wrap it round your head to ward off noxious fumes or avoid the gaze of the Ravenous Bugblatter Beast of Traal " \
    "(such a mind-boggingly stupid animal, it assumes that if you can't see it, it can't see you); " \
    "you can wave your towel in emergencies as a distress signal, and of course dry yourself off with it if it still seems to be clean enough."
}};

TextRenderApp::TextRenderApp()
    : GraphicsApp(
        Samples::GetAppSettings("Methane Text Rendering", true /* animations */, true /* logo */, true /* hud ui */, false /* depth */),
        "Methane tutorial of text rendering")
    , m_displayed_text_lengths(g_text_blocks_count, 1)
{
    GetHeadsUpDisplaySettings().position = gfx::Point2i(g_margin_size_in_dots, g_margin_size_in_dots);

    // Setup animations
    m_animations.push_back(std::make_shared<Data::TimeAnimation>(std::bind(&TextRenderApp::Animate, this, std::placeholders::_1, std::placeholders::_2)));
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

    const gfx::FrameSize frame_size_in_dots = GetFrameSizeInDots();
    const uint32_t frame_width_without_margins = frame_size_in_dots.width - 2 * g_margin_size_in_dots;
    int32_t vertical_text_pos_in_dots = g_top_text_pos_in_dots;

    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        const FontSettings& font_settings = g_font_settings[block_index];
        const std::string displayed_text_block = g_text_blocks[block_index].substr(0, m_displayed_text_lengths[block_index]);

        // Add font to library
        m_fonts.push_back(
            gfx::Font::Library::Get().AddFont(
                Data::FontProvider::Get(),
                gfx::Font::Settings
                {
                    font_settings.name, font_settings.path, font_settings.size, m_sp_context->GetFontResolutionDPI(),
                    gfx::Font::GetTextAlphabet(displayed_text_block)
                }
            ).GetPtr()
        );

        // Add text element
        m_texts.push_back(
            std::make_shared<gfx::Text>(*m_sp_context, *m_fonts.back(),
                gfx::Text::Settings
                {
                    font_settings.name,
                    displayed_text_block,
                    gfx::FrameRect
                    {
                        { g_margin_size_in_dots, vertical_text_pos_in_dots },
                        { frame_width_without_margins, 0u /* calculated height */ }
                    },
                    false, // screen_rect_in_pixels
                    gfx::Color4f(font_settings.color, 1.f),
                    gfx::Text::Wrap::Word,
                }
            )
        );
    }

    UpdateFontAtlasBadges();

    // Create per-frame command lists
    for(TextRenderFrame& frame : m_frames)
    {
        frame.sp_render_cmd_list = gfx::RenderCommandList::Create(m_sp_context->GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_render_cmd_list->SetName(IndexedName("Text Rendering", frame.index));
        frame.sp_execute_cmd_lists = gfx::CommandListSet::Create({ *frame.sp_render_cmd_list });
    }

    CompleteInitialization();
}

Ptr<gfx::Badge> TextRenderApp::CreateFontAtlasBadge(gfx::Font& font, const Ptr<gfx::Texture>& sp_atlas_texture)
{
    const auto font_color_by_name_it = g_font_color_by_name.find(font.GetSettings().name);
    const gfx::Color3f& font_color = font_color_by_name_it != g_font_color_by_name.end()
                                   ? font_color_by_name_it->second : g_misc_font_color;

    return std::make_shared<gfx::Badge>(
        *m_sp_context, sp_atlas_texture,
        gfx::Badge::Settings
        {
            static_cast<const gfx::FrameSize&>(sp_atlas_texture->GetSettings().dimensions),
            gfx::Badge::FrameCorner::BottomLeft,
            gfx::Point2u(16u, 16u),
            gfx::Color4f(font_color, 0.5f),
            gfx::Badge::TextureMode::RFloatToAlpha
        }
    );
}

void TextRenderApp::UpdateFontAtlasBadges()
{
    const Refs<gfx::Font> font_refs = gfx::Font::Library::Get().GetFonts();
    gfx::Context& context = *m_sp_context;

    // Remove obsolete font atlas badges
    for(auto badge_it = m_font_atlas_badges.begin(); badge_it != m_font_atlas_badges.end();)
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
            badge_it = m_font_atlas_badges.erase(badge_it);
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
        const auto sp_font_atlas_it = std::find_if(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                                                   [&sp_font_atlas_texture](const Ptr<gfx::Badge>& sp_font_atlas_badge)
            {
                return std::addressof(sp_font_atlas_badge->GetTexture()) == sp_font_atlas_texture.get();
            }
        );

        if (sp_font_atlas_it != m_font_atlas_badges.end())
            continue;

        font_ref.get().Connect(*this);

        m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font_ref.get(), sp_font_atlas_texture));
    }

    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

void TextRenderApp::LayoutFontAtlasBadges(const gfx::FrameSize& frame_size)
{
    // Sort atlas badges by size so that largest are displayed first
    std::sort(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
              [](const Ptr<gfx::Badge>& sp_left, const Ptr<gfx::Badge>& sp_right)
              {
                  return sp_left->GetSettings().screen_rect.size.GetPixelsCount() >
                         sp_right->GetSettings().screen_rect.size.GetPixelsCount();
              }
    );

    const float scale_factor = GetRenderContext().GetContentScalingFactor();
    gfx::Point2i badge_margins(g_margin_size_in_dots, g_margin_size_in_dots);
    badge_margins *= scale_factor;

    // Layout badges in a row one after another with a margin spacing
    for(const Ptr<gfx::Badge>& sp_badge_atlas : m_font_atlas_badges)
    {
        assert(sp_badge_atlas);
        const gfx::FrameSize& atlas_size = static_cast<const gfx::FrameSize&>(sp_badge_atlas->GetTexture().GetSettings().dimensions);
        sp_badge_atlas->FrameResize(frame_size, atlas_size, badge_margins);
        badge_margins.SetX(badge_margins.GetX() + atlas_size.width + static_cast<int32_t>(scale_factor * g_margin_size_in_dots));
    }
}

bool TextRenderApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!GraphicsApp::Resize(frame_size, is_minimized))
        return false;

    const gfx::FrameSize frame_size_in_dots    = GetFrameSizeInDots();
    const uint32_t frame_width_without_margins = frame_size_in_dots.width - 2 * g_margin_size_in_dots;
    int32_t        vertical_text_pos_in_dots   = g_top_text_pos_in_dots;

    for(Ptr<gfx::Text>& sp_text : m_texts)
    {
        sp_text->SetScreenRect({
            { g_margin_size_in_dots, vertical_text_pos_in_dots },
            { frame_width_without_margins, 0u /* calculated height */ }
        });
        vertical_text_pos_in_dots += sp_text->GetViewportInDots().size.height + g_margin_size_in_dots;
    }

    LayoutFontAtlasBadges(frame_size);
    return true;
}

bool TextRenderApp::Animate(double elapsed_seconds, double)
{
    if (elapsed_seconds - m_text_update_elapsed_sec < g_text_update_interval_sec)
        return true;

    m_text_update_elapsed_sec = elapsed_seconds;

    int32_t vertical_text_pos_in_dots = g_top_text_pos_in_dots;
    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        size_t& displayed_text_length = m_displayed_text_lengths[block_index];
        displayed_text_length = displayed_text_length < g_text_blocks[block_index].length() - 1 ? displayed_text_length + 1 : 1;
        const std::string displayed_text = g_text_blocks[block_index].substr(0, displayed_text_length);

        const Ptr<gfx::Text>& sp_text = m_texts[block_index];
        sp_text->SetTextInScreenRect(displayed_text, {
            { g_margin_size_in_dots, vertical_text_pos_in_dots  },
            { GetFrameSizeInDots().width - 2 * g_margin_size_in_dots, 0u }
        });

        vertical_text_pos_in_dots = sp_text->GetViewportInDots().GetBottom() + g_margin_size_in_dots;

        if (displayed_text_length == 1)
        {
            m_fonts[block_index]->ResetChars(gfx::Font::GetTextAlphabet(displayed_text));
        }
    }

    return true;
}

bool TextRenderApp::Render()
{
    // Render only when context is ready
    if (!m_sp_context->ReadyToRender() || !GraphicsApp::Render())
        return false;

    // Draw text blocks
    TextRenderFrame& frame = GetCurrentFrame();
    for(Ptr<gfx::Text>& sp_text : m_texts)
    {
        sp_text->Draw(*frame.sp_render_cmd_list);
    }

    // Draw font atlas badges
    for(const Ptr<gfx::Badge>& sp_badge_atlas : m_font_atlas_badges)
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

    m_fonts.clear();
    m_texts.clear();
    m_font_atlas_badges.clear();

    GraphicsApp::OnContextReleased(context);
}

void TextRenderApp::OnFontAtlasTextureReset(gfx::Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    const auto sp_font_atlas_badge_it = std::find_if(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                                                     [&sp_old_atlas_texture](const Ptr<gfx::Badge>& sp_font_atlas_badge)
        {
           return std::addressof(sp_font_atlas_badge->GetTexture()) == sp_old_atlas_texture.get();
        }
    );

    if (sp_font_atlas_badge_it == m_font_atlas_badges.end())
    {
        m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font, sp_new_atlas_texture));
    }
    else
    {
        Ptr<gfx::Badge>& sp_badge = *sp_font_atlas_badge_it;
        sp_badge->SetTexture(sp_new_atlas_texture);
        sp_badge->SetSize(static_cast<const gfx::FrameSize&>(sp_new_atlas_texture->GetSettings().dimensions));
    }
}

void TextRenderApp::OnFontAtlasUpdated(gfx::Font&)
{
    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TextRenderApp().Run({ argc, argv });
}
