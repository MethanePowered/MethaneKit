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

FILE: TypographyApp.cpp
Tutorial demonstrating dynamic text rendering and fonts management with Methane Kit.

******************************************************************************/

#include "TypographyApp.h"
#include "TypographyAppController.h"

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <array>

namespace Methane::Tutorials
{

struct FontSettings
{
    gui::Font::Description desc;
    gfx::Color3f           color;
};

constexpr int32_t g_margin_size_in_dots      = 32;
constexpr int32_t g_top_text_pos_in_dots     = 100;
constexpr size_t  g_text_blocks_count        = 3;

static const std::array<FontSettings, g_text_blocks_count> g_font_settings { {
    { { "European",     "Fonts/Roboto/Roboto-Regular.ttf",                 20u }, { 1.f,  1.f,  0.5f } },
    { { "Japanese",     "Fonts/SawarabiMincho/SawarabiMincho-Regular.ttf", 20u }, { 1.f,  0.3f, 0.1f } },
//  { { "Arabic",       "Fonts/Amiri/Amiri-Regular.ttf",                   20u }, { 1.f,  0.3f, 0.8f } },
    { { "Calligraphic", "Fonts/Playball/Playball-Regular.ttf",             20u }, { 0.5f, 1.f,  0.5f } }
} };

static const gfx::Color3f g_misc_font_color { 1.f, 1.f, 1.f };
static const std::map<std::string, gfx::Color3f>    g_font_color_by_name   {
    { g_font_settings[0].desc.name, g_font_settings[0].color },
    { g_font_settings[1].desc.name, g_font_settings[1].color },
    { g_font_settings[2].desc.name, g_font_settings[2].color },
//  { g_font_settings[3].desc.name, g_font_settings[3].color }
};

// Pangrams from http://clagnut.com/blog/2380/
static const std::array<std::u32string, g_text_blocks_count> g_text_blocks = { {

    // 0: european pangrams
    gui::Font::ConvertUtf8To32(
        "The quick brown fox jumps over the lazy dog!\n" \
         "Съешь ещё этих мягких французских булок, да выпей чаю.\n" \
         "Ο καλύμνιος σφουγγαράς ψιθύρισε πως θα βουτήξει χωρίς να διστάζει.\n" \
         "Pijamalı hasta, yağız şoföre çabucak güvendi."),

    // 1: japanese pangram
    gui::Font::ConvertUtf8To32(
        "いろはにほへと ちりぬるを わかよたれそ つねならむ うゐのおくやま けふこえて あさきゆめみし ゑひもせす"),

    // 2: arabic pangram
    //gui::Font::ConvertUtf8To32(
    //  "نص حكيم له سر قاطع وذو شأن عظيم مكتوب على ثوب أخضر ومغلف بجلد أزرق"),

    // 3: hitchhicker's guide quote
    gui::Font::ConvertUtf8To32(
        "A towel is about the most massively useful thing an interstellar hitchhiker can have. " \
        "Partly it has great practical value. You can wrap it around you for warmth as you bound across the cold moons of Jaglan Beta; " \
        "you can lie on it on the brilliant marble-sanded beaches of Santraginus V, inhaling the heady sea vapors; " \
        "you can sleep under it beneath the stars which shine so redly on the desert world of Kakrafoon; " \
        "use it to sail a miniraft down the slow heavy River Moth; " \
        "wet it for use in hand-to-hand-combat; " \
        "wrap it round your head to ward off noxious fumes or avoid the gaze of the Ravenous Bugblatter Beast of Traal " \
        "(such a mind-boggingly stupid animal, it assumes that if you can't see it, it can't see you); " \
        "you can wave your towel in emergencies as a distress signal, and of course dry yourself off with it if it still seems to be clean enough.")
}};

namespace pal = Methane::Platform;
static const std::map<pal::Keyboard::State, TypographyAppAction> g_typography_action_by_keyboard_state{
    { { pal::Keyboard::Key::W       }, TypographyAppAction::SwitchTextWrapMode            },
    { { pal::Keyboard::Key::H       }, TypographyAppAction::SwitchTextHorizontalAlignment },
    { { pal::Keyboard::Key::V       }, TypographyAppAction::SwitchTextVerticalAlignment   },
    { { pal::Keyboard::Key::U       }, TypographyAppAction::SwitchIncrementalTextUpdate   },
    { { pal::Keyboard::Key::D       }, TypographyAppAction::SwitchTypingDirection         },
    { { pal::Keyboard::Key::Equal   }, TypographyAppAction::SpeedupTyping                 },
    { { pal::Keyboard::Key::Minus   }, TypographyAppAction::SlowdownTyping                },
};

TypographyApp::TypographyApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Typography", true /* animations */, false /* depth */),
        { gui::IApp::HeadsUpDisplayMode::UserInterface, true },
        "Dynamic text rendering and fonts management tutorial.")
    , m_displayed_text_lengths(g_text_blocks_count, 0)
{
    m_displayed_text_lengths[0] = 1;
    GetHeadsUpDisplaySettings().position = gui::UnitPoint(g_margin_size_in_dots, g_margin_size_in_dots, gui::Units::Dots);

    gui::Font::Library::Get().Connect(*this);
    AddInputControllers({
        std::make_shared<TypographyAppController>(*this, g_typography_action_by_keyboard_state)
    });

    // Setup animations
    GetAnimations().push_back(
        std::make_shared<Data::TimeAnimation>(std::bind(&TypographyApp::Animate, this, std::placeholders::_1, std::placeholders::_2))
    );
}

TypographyApp::~TypographyApp()
{
    // Wait for GPU rendering is completed to release resources
    GetRenderContext().WaitForGpu(gfx::Context::WaitFor::RenderComplete);

    // Clear the font library to release all atlas textures
    gui::Font::Library::Get().Clear();
}

void TypographyApp::Init()
{
    UserInterfaceApp::Init();

    const gfx::FrameSize frame_size_in_dots = GetFrameSizeInDots();
    const uint32_t frame_width_without_margins = frame_size_in_dots.width - 2 * g_margin_size_in_dots;
    int32_t vertical_text_pos_in_dots = g_top_text_pos_in_dots;

    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        const FontSettings& font_settings = g_font_settings[block_index];
        const size_t displayed_text_length = m_displayed_text_lengths[block_index];
        const std::u32string displayed_text_block = g_text_blocks[block_index].substr(0, displayed_text_length);

        // Add font to library
        m_fonts.push_back(
            gui::Font::Library::Get().AddFont(
                Data::FontProvider::Get(),
                gui::Font::Settings
                {
                    font_settings.desc, GetUIContext().GetFontResolutionDpi(),
                    gui::Font::GetAlphabetFromText(displayed_text_block)
                }
            ).GetPtr()
        );

        // Add text element
        m_texts.push_back(
            std::make_shared<gui::Text>(GetUIContext(), *m_fonts.back(),
                gui::Text::SettingsUtf32
                {
                    font_settings.desc.name,
                    displayed_text_block,
                    gui::UnitRect
                    {
                        { g_margin_size_in_dots, vertical_text_pos_in_dots },
                        { frame_width_without_margins, 0u /* calculated height */ },
                        gui::Units::Dots
                    },
                    m_settings.text_layout,
                    gfx::Color4f(font_settings.color, 1.f),
                    m_settings.is_incremental_text_update
                }
            )
        );

        vertical_text_pos_in_dots = m_texts.back()->GetContentRectInDots().GetBottom() + g_margin_size_in_dots;
    }

    UpdateFontAtlasBadges();

    // Create per-frame command lists
    for(TypographyFrame& frame : GetFrames())
    {
        frame.sp_render_cmd_list = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.sp_screen_pass);
        frame.sp_render_cmd_list->SetName(IndexedName("Text Rendering", frame.index));
        frame.sp_execute_cmd_list_set = gfx::CommandListSet::Create({ *frame.sp_render_cmd_list });
    }

    CompleteInitialization();
}

Ptr<gui::Badge> TypographyApp::CreateFontAtlasBadge(gui::Font& font, const Ptr<gfx::Texture>& sp_atlas_texture)
{
    const auto font_color_by_name_it = g_font_color_by_name.find(font.GetSettings().description.name);
    const gui::Color3f& font_color = font_color_by_name_it != g_font_color_by_name.end()
                                   ? font_color_by_name_it->second : g_misc_font_color;

    return std::make_shared<gui::Badge>(
        GetUIContext(), sp_atlas_texture,
        gui::Badge::Settings
        {
            font.GetSettings().description.name + " Font Atlas",
            gui::UnitSize(static_cast<const gfx::FrameSize&>(sp_atlas_texture->GetSettings().dimensions), gui::Units::Pixels),
            gui::Badge::FrameCorner::BottomLeft,
            gui::UnitPoint(16u, 16u, gui::Units::Dots),
            gui::Color4f(font_color, 0.5f),
            gui::Badge::TextureMode::Volatile,
            gui::Badge::TextureColorMode::RFloatToAlpha,
        }
    );
}

void TypographyApp::UpdateFontAtlasBadges()
{
    const Refs<gui::Font> font_refs = gui::Font::Library::Get().GetFonts();
    gfx::RenderContext& context = GetRenderContext();

    // Remove obsolete font atlas badges
    for(auto badge_it = m_font_atlas_badges.begin(); badge_it != m_font_atlas_badges.end();)
    {
        const Ptr<gui::Badge>& sp_font_atlas_badge = *badge_it;
        const auto font_ref_it = std::find_if(font_refs.begin(), font_refs.end(),
            [&sp_font_atlas_badge, &context](const Ref<gui::Font>& font_ref)
            {
                return std::addressof(sp_font_atlas_badge->GetTexture()) == font_ref.get().GetAtlasTexturePtr(context).get();
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
    for(const Ref<gui::Font>& font_ref : font_refs)
    {
        const Ptr<gfx::Texture>& sp_font_atlas_texture = font_ref.get().GetAtlasTexturePtr(context);
        if (!sp_font_atlas_texture)
            continue;

        const auto sp_font_atlas_it = std::find_if(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                                                   [&sp_font_atlas_texture](const Ptr<gui::Badge>& sp_font_atlas_badge)
            {
                return std::addressof(sp_font_atlas_badge->GetTexture()) == sp_font_atlas_texture.get();
            }
        );

        if (sp_font_atlas_it != m_font_atlas_badges.end())
            continue;

        m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font_ref.get(), sp_font_atlas_texture));
    }

    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

void TypographyApp::LayoutFontAtlasBadges(const gfx::FrameSize& frame_size)
{
    // Sort atlas badges by size so that largest are displayed first
    std::sort(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
              [](const Ptr<gui::Badge>& sp_left, const Ptr<gui::Badge>& sp_right)
              {
                  return sp_left->GetSettings().screen_rect.size.GetPixelsCount() >
                         sp_right->GetSettings().screen_rect.size.GetPixelsCount();
              }
    );

    // Layout badges in a row one after another with a margin spacing
    gui::UnitPoint badge_margins(g_margin_size_in_dots, g_margin_size_in_dots, gui::Units::Dots);
    for(const Ptr<gui::Badge>& sp_badge_atlas : m_font_atlas_badges)
    {
        assert(sp_badge_atlas);
        const gui::UnitSize atlas_size = GetUIContext().ConvertToDots(gui::UnitSize(static_cast<const gfx::FrameSize&>(sp_badge_atlas->GetTexture().GetSettings().dimensions), gui::Units::Pixels));
        sp_badge_atlas->FrameResize(gui::UnitSize(frame_size, gui::Units::Pixels), atlas_size, badge_margins);
        badge_margins += gui::UnitPoint(atlas_size.width + static_cast<int32_t>(g_margin_size_in_dots), 0u, gui::Units::Dots);
    }
}

bool TypographyApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    const gfx::FrameSize frame_size_in_dots    = GetFrameSizeInDots();
    const uint32_t frame_width_without_margins = frame_size_in_dots.width - 2 * g_margin_size_in_dots;
    int32_t        vertical_text_pos_in_dots   = g_top_text_pos_in_dots;

    for(Ptr<gui::Text>& sp_text : m_texts)
    {
        sp_text->SetRect(gui::UnitRect(
            { g_margin_size_in_dots, vertical_text_pos_in_dots },
            { frame_width_without_margins, 0u /* calculated height */ },
            gui::Units::Dots
        ));
        vertical_text_pos_in_dots += sp_text->GetContentRectInDots().size.height + g_margin_size_in_dots;
    }

    LayoutFontAtlasBadges(frame_size);
    return true;
}

bool TypographyApp::Animate(double elapsed_seconds, double)
{
    if (elapsed_seconds - m_text_update_elapsed_sec < m_settings.typing_update_interval_sec)
        return true;

    m_text_update_elapsed_sec = elapsed_seconds;

    int32_t vertical_text_pos_in_dots = g_top_text_pos_in_dots;
    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        size_t& displayed_text_length    = m_displayed_text_lengths[block_index];
        const Ptr<gui::Text>& sp_text    = m_texts[block_index];
        const std::u32string& text_block = g_text_blocks[block_index];
        const size_t   text_block_length = text_block.length();

        if (displayed_text_length == (m_settings.is_forward_typing_direction ? 0 : text_block_length))
        {
            sp_text->SetText(m_settings.is_forward_typing_direction ? std::u32string() : text_block);
            if (!m_settings.is_forward_typing_direction)
                vertical_text_pos_in_dots = sp_text->GetContentRectInDots().GetBottom() + g_margin_size_in_dots;
            continue;
        }

        if (displayed_text_length == (m_settings.is_forward_typing_direction ? text_block_length : 0))
        {
            if (block_index == (m_settings.is_forward_typing_direction ? g_text_blocks_count - 1 : 0))
            {
                ResetAnimation();
            }
            else
            {
                vertical_text_pos_in_dots = sp_text->GetContentRectInDots().GetBottom() + g_margin_size_in_dots;
                size_t next_block_index = block_index + (m_settings.is_forward_typing_direction ? 1 : -1);
                size_t& next_displayed_text_length = m_displayed_text_lengths[next_block_index];
                if (m_settings.is_forward_typing_direction && next_displayed_text_length == 0)
                    next_displayed_text_length = 1;
                if (!m_settings.is_forward_typing_direction && next_displayed_text_length == g_text_blocks[next_block_index].length())
                    next_displayed_text_length = g_text_blocks[next_block_index].length() - 1;
            }
            continue;
        }

        if (m_settings.is_forward_typing_direction)
            displayed_text_length++;
        else
            displayed_text_length--;

        const std::u32string displayed_text = text_block.substr(0, displayed_text_length);
        {
            Methane::ScopeTimer scope_timer("Text update");
            sp_text->SetTextInScreenRect(displayed_text, gui::UnitRect(
                { g_margin_size_in_dots, vertical_text_pos_in_dots  },
                { GetFrameSizeInDots().width - 2 * g_margin_size_in_dots, 0u },
                gui::Units::Dots
            ));
            m_text_update_duration = scope_timer.GetElapsedDuration();
        }
        vertical_text_pos_in_dots = sp_text->GetContentRectInDots().GetBottom() + g_margin_size_in_dots;
    }

    UpdateParametersText();
    return true;
}

void TypographyApp::ResetAnimation()
{
    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        const size_t displayed_text_length  = m_settings.is_forward_typing_direction
                                            ? (block_index ? 0 : 1)
                                            : (g_text_blocks[block_index].length() - (block_index == g_text_blocks_count - 1 ? 1 : 0));
        const std::u32string displayed_text = g_text_blocks[block_index].substr(0, displayed_text_length);
        m_displayed_text_lengths[block_index] = displayed_text_length;
        m_texts[block_index]->SetText(displayed_text);
        m_fonts[block_index]->ResetChars(displayed_text);
    }
    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

bool TypographyApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    TypographyFrame& frame = GetCurrentFrame();

    // Draw text blocks
    for(Ptr<gui::Text>& sp_text : m_texts)
    {
        sp_text->Draw(*frame.sp_render_cmd_list);
    }

    // Draw font atlas badges
    for(const Ptr<gui::Badge>& sp_badge_atlas : m_font_atlas_badges)
    {
        sp_badge_atlas->Draw(*frame.sp_render_cmd_list);
    }

    RenderOverlay(*frame.sp_render_cmd_list);

    // Commit command list with present flag
    frame.sp_render_cmd_list->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.sp_execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}

std::string TypographyApp::GetParametersString()
{
    std::stringstream ss;
    ss << "Typography demo parameters:"
       << std::endl << "  - text wrap mode:            " << gui::Text::GetWrapName(m_settings.text_layout.wrap)
       << std::endl << "  - horizontal text alignment: " << gui::Text::GetHorizontalAlignmentName(m_settings.text_layout.horizontal_alignment)
       << std::endl << "  - vertical text alignment:   " << gui::Text::GetVerticalAlignmentName(m_settings.text_layout.vertical_alignment)
       << std::endl << "  - text typing mode:          " << (m_settings.is_forward_typing_direction ? "Appending" : "Backspace")
       << std::endl << "  - text typing interval (ms): " << static_cast<uint32_t>(m_settings.typing_update_interval_sec * 1000)
       << std::endl << "  - text typing animation:     " << (!GetAnimations().IsPaused() ? "ON" : "OFF")
       << std::endl << "  - incremental text updates:  " << (m_settings.is_incremental_text_update ? "ON" : "OFF")
       << std::endl << "  - text update duration (us): " << static_cast<double>(m_text_update_duration.count()) / 1000;

    return ss.str();
}

void TypographyApp::SetTextLayout(const gui::Text::Layout& text_layout)
{
    if (m_settings.text_layout == text_layout)
        return;

    m_settings.text_layout = text_layout;
    for (const Ptr<gui::Text>& sp_text : m_texts)
    {
        sp_text->SetLayout(text_layout);
    }

    UpdateParametersText();
}

void TypographyApp::SetForwardTypingDirection(bool is_forward_typing_direction)
{
    if (m_settings.is_forward_typing_direction == is_forward_typing_direction)
        return;

    m_settings.is_forward_typing_direction = is_forward_typing_direction;
    UpdateParametersText();
}

void TypographyApp::SetTextUpdateInterval(double text_update_interval_sec)
{
    if (m_settings.typing_update_interval_sec == text_update_interval_sec)
        return;

    m_settings.typing_update_interval_sec = text_update_interval_sec;
    UpdateParametersText();
}

void TypographyApp::SetIncrementalTextUpdate(bool is_incremental_text_update)
{
    if (m_settings.is_incremental_text_update == is_incremental_text_update)
        return;

    m_settings.is_incremental_text_update = is_incremental_text_update;
    for(const Ptr<gui::Text>& sp_text : m_texts)
    {
        sp_text->SetIncrementalUpdate(is_incremental_text_update);
    }

    UpdateParametersText();
}

void TypographyApp::OnContextReleased(gfx::Context& context)
{
    gui::Font::Library::Get().Clear();

    m_fonts.clear();
    m_texts.clear();
    m_font_atlas_badges.clear();

    UserInterfaceApp::OnContextReleased(context);
}

void TypographyApp::OnFontAdded(gui::Font& font)
{
    font.Connect(*this);
}

void TypographyApp::OnFontAtlasTextureReset(gui::Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture)
{
    const auto sp_font_atlas_badge_it = std::find_if(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                                                     [&sp_old_atlas_texture](const Ptr<gui::Badge>& sp_font_atlas_badge)
        {
           return std::addressof(sp_font_atlas_badge->GetTexture()) == sp_old_atlas_texture.get();
        }
    );

    if (sp_new_atlas_texture)
    {
        if (sp_font_atlas_badge_it == m_font_atlas_badges.end())
        {
            m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font, sp_new_atlas_texture));
            LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
        }
        else
        {
            Ptr<gui::Badge>& sp_badge = *sp_font_atlas_badge_it;
            sp_badge->SetTexture(sp_new_atlas_texture);
            sp_badge->SetSize(gui::UnitSize(static_cast<const gfx::FrameSize&>(sp_new_atlas_texture->GetSettings().dimensions), gui::Units::Pixels));
        }
    }
    else if (sp_font_atlas_badge_it != m_font_atlas_badges.end())
    {
        m_font_atlas_badges.erase(sp_font_atlas_badge_it);
        LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
    }
}

void TypographyApp::OnFontAtlasUpdated(gui::Font&)
{
    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::TypographyApp().Run({ argc, argv });
}
