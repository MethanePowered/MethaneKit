/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: TypographyApp.cpp
Tutorial demonstrating dynamic text rendering and fonts management with Methane Kit.

******************************************************************************/

#include "TypographyApp.h"
#include "TypographyAppController.h"

#include <Methane/Samples/AppSettings.hpp>
#include <Methane/Data/TimeAnimation.h>

#include <magic_enum.hpp>
#include <array>

namespace Methane::Tutorials
{

struct FontSettings
{
    gui::Font::Description desc;
    gfx::Color3f           color;
};

constexpr int32_t g_margin_size_in_dots  = 32;
constexpr int32_t g_top_text_pos_in_dots = 110;
constexpr size_t  g_text_blocks_count    = 3;

static const std::array<FontSettings, g_text_blocks_count> g_font_settings { {
    { { "European",     "Fonts/Roboto/Roboto-Regular.ttf",                 20U }, { 1.F,  1.F,  0.5F } },
    { { "Japanese",     "Fonts/SawarabiMincho/SawarabiMincho-Regular.ttf", 20U }, { 1.F,  0.3F, 0.1F } },
    { { "Calligraphic", "Fonts/Playball/Playball-Regular.ttf",             20U }, { 0.5F, 1.F,  0.5F } }
} };

static const gfx::Color3f g_misc_font_color { 1.F, 1.F, 1.F };
static const std::map<std::string, gfx::Color3f> g_font_color_by_name   {
    { g_font_settings[0].desc.name, g_font_settings[0].color },
    { g_font_settings[1].desc.name, g_font_settings[1].color },
    { g_font_settings[2].desc.name, g_font_settings[2].color },
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

    // 2: hitchhicker's guide quote
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

static gui::UnitRect GetTextBlockRectInDots(size_t block_index, int32_t vertical_pos_in_dots, const gfx::FrameSize& frame_size_in_dots)
{
    return gui::UnitRect(
        gui::Units::Dots,
        gfx::Point2i
        {
            g_margin_size_in_dots,
            vertical_pos_in_dots
        },
        gfx::FrameSize
        {
            frame_size_in_dots.width - 2 * g_margin_size_in_dots,
            block_index == g_text_blocks_count - 1
            ? frame_size_in_dots.height - vertical_pos_in_dots - g_margin_size_in_dots  // last text block fills all available space
            : 0U                                                                        // other text blocks have calculated height
        }
    );
}

inline Timer::TimeDuration UpdateTextRect(gui::Text& text, const gui::UnitRect& text_block_rect)
{
    Methane::ScopeTimer scope_timer("Text update");
    text.SetRect(text_block_rect);
    return scope_timer.GetElapsedDuration();
}

inline Timer::TimeDuration UpdateText(gui::Text& text, const std::u32string& displayed_text, const gui::UnitRect& text_block_rect)
{
    Methane::ScopeTimer scope_timer("Text update");
    text.SetTextInScreenRect(displayed_text, text_block_rect);
    return scope_timer.GetElapsedDuration();
}

TypographyApp::TypographyApp()
    : UserInterfaceApp(
        Samples::GetGraphicsAppSettings("Methane Typography", true /* animations */, false /* depth */),
        { gui::IApp::HeadsUpDisplayMode::UserInterface, true },
        "Dynamic text rendering and fonts management tutorial.")
{
    m_displayed_text_lengths.resize(g_text_blocks_count, 0);
    m_displayed_text_lengths[0] = 1;

    GetHeadsUpDisplaySettings().position = gui::UnitPoint(gui::Units::Dots, g_margin_size_in_dots, g_margin_size_in_dots);

    gui::Font::Library::Get().Connect(*this);
    AddInputControllers({
        std::make_shared<TypographyAppController>(*this, g_typography_action_by_keyboard_state)
    });

    // Setup animations
    GetAnimations().push_back(
        std::make_shared<Data::TimeAnimation>(std::bind(&TypographyApp::Animate, this, std::placeholders::_1, std::placeholders::_2))
    );

    ShowParameters();
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
                        gui::Units::Dots,
                        gfx::Point2i { g_margin_size_in_dots, vertical_text_pos_in_dots },
                        gfx::FrameSize { frame_width_without_margins, 0U /* calculated height */ }
                    },
                    m_settings.text_layout,
                    gfx::Color4f(font_settings.color, 1.F),
                    m_settings.is_incremental_text_update
                }
            )
        );

        vertical_text_pos_in_dots = m_texts.back()->GetRectInDots().GetBottom() + g_margin_size_in_dots;
    }

    UpdateFontAtlasBadges();

    // Create per-frame command lists
    for(TypographyFrame& frame : GetFrames())
    {
        frame.render_cmd_list_ptr = gfx::RenderCommandList::Create(GetRenderContext().GetRenderCommandQueue(), *frame.screen_pass_ptr);
        frame.render_cmd_list_ptr->SetName(IndexedName("Text Rendering", frame.index));
        frame.execute_cmd_list_set_ptr = gfx::CommandListSet::Create({ *frame.render_cmd_list_ptr });
    }

    CompleteInitialization();
}

Ptr<gui::Badge> TypographyApp::CreateFontAtlasBadge(const gui::Font& font, const Ptr<gfx::Texture>& atlas_texture_ptr)
{
    const auto font_color_by_name_it = g_font_color_by_name.find(font.GetSettings().description.name);
    const gui::Color3f& font_color = font_color_by_name_it != g_font_color_by_name.end()
                                   ? font_color_by_name_it->second : g_misc_font_color;

    return std::make_shared<gui::Badge>(
        GetUIContext(), atlas_texture_ptr,
        gui::Badge::Settings
        {
            font.GetSettings().description.name + " Font Atlas",
            gui::UnitSize(gui::Units::Pixels, static_cast<const gfx::FrameSize&>(atlas_texture_ptr->GetSettings().dimensions)),
            gui::Badge::FrameCorner::BottomLeft,
            gui::UnitPoint(gui::Units::Dots, 16U, 16U),
            gui::Color4f(font_color, 0.5F),
            gui::Badge::TextureMode::RFloatToAlpha,
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
        const Ptr<gui::Badge>& font_atlas_badge_ptr = *badge_it;
        const auto font_ref_it = std::find_if(font_refs.begin(), font_refs.end(),
            [&font_atlas_badge_ptr, &context](const Ref<gui::Font>& font_ref)
            {
                return std::addressof(font_atlas_badge_ptr->GetTexture()) == font_ref.get().GetAtlasTexturePtr(context).get();
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
        const Ptr<gfx::Texture>& font_atlas_texture_ptr = font_ref.get().GetAtlasTexturePtr(context);
        if (!font_atlas_texture_ptr)
            continue;

        const auto font_atlas_ptr_it = std::find_if(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                                                   [&font_atlas_texture_ptr](const Ptr<gui::Badge>& font_atlas_badge_ptr)
            {
                return std::addressof(font_atlas_badge_ptr->GetTexture()) == font_atlas_texture_ptr.get();
            }
        );

        if (font_atlas_ptr_it != m_font_atlas_badges.end())
            continue;

        m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font_ref.get(), font_atlas_texture_ptr));
    }

    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

void TypographyApp::LayoutFontAtlasBadges(const gfx::FrameSize& frame_size)
{
    // Sort atlas badges by size so that largest are displayed first
    std::sort(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
              [](const Ptr<gui::Badge>& left_ptr, const Ptr<gui::Badge>& right_ptr)
              {
                  return left_ptr->GetSettings().screen_rect.size.GetPixelsCount() >
                         right_ptr->GetSettings().screen_rect.size.GetPixelsCount();
              }
    );

    // Layout badges in a row one after another with a margin spacing
    gui::UnitPoint badge_margins(gui::Units::Dots, g_margin_size_in_dots, g_margin_size_in_dots);
    for(const Ptr<gui::Badge>& badge_atlas_ptr : m_font_atlas_badges)
    {
        META_CHECK_ARG_NOT_NULL(badge_atlas_ptr);
        const gui::UnitSize atlas_size = GetUIContext().ConvertToDots(gui::UnitSize(gui::Units::Pixels, static_cast<const gfx::FrameSize&>(badge_atlas_ptr->GetTexture().GetSettings().dimensions)));
        badge_atlas_ptr->FrameResize(gui::UnitSize(gui::Units::Pixels, frame_size), atlas_size, badge_margins);
        badge_margins += gui::UnitPoint(gui::Units::Dots, atlas_size.width + g_margin_size_in_dots, 0U);
    }
}

bool TypographyApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    const gfx::FrameSize frame_size_in_dots = GetFrameSizeInDots();
    int32_t       vertical_text_pos_in_dots = g_top_text_pos_in_dots;

    for (size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        const Ptr<gui::Text>& text_ptr = m_texts[block_index];
        const gui::UnitRect text_block_rect = GetTextBlockRectInDots(block_index, vertical_text_pos_in_dots, frame_size_in_dots);
        m_text_update_duration = UpdateTextRect(*text_ptr, text_block_rect);
        vertical_text_pos_in_dots += text_ptr->GetRectInDots().size.height + g_margin_size_in_dots;
    }

    LayoutFontAtlasBadges(frame_size);
    UpdateParametersText(); // show text update timing
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
        AnimateTextBlock(block_index, vertical_text_pos_in_dots);
    }

    UpdateParametersText();
    return true;
}

void TypographyApp::AnimateTextBlock(size_t block_index, int32_t& vertical_text_pos_in_dots)
{
    gui::Text& text                 = *m_texts[block_index];
    const std::u32string& full_text = g_text_blocks[block_index];
    const size_t text_block_length  = full_text.length();
    size_t& displayed_text_length   = m_displayed_text_lengths[block_index];

    if (displayed_text_length == (m_settings.is_forward_typing_direction ? 0 : text_block_length))
    {
        text.SetText(m_settings.is_forward_typing_direction ? std::u32string() : full_text);
        if (!m_settings.is_forward_typing_direction)
            vertical_text_pos_in_dots = text.GetRectInDots().GetBottom() + g_margin_size_in_dots;
        return;
    }

    if (displayed_text_length == (m_settings.is_forward_typing_direction ? text_block_length : 0))
    {
        if (block_index == (m_settings.is_forward_typing_direction ? g_text_blocks_count - 1 : 0))
        {
            ResetAnimation();
            return;
        }

        vertical_text_pos_in_dots = text.GetRectInDots().GetBottom() + g_margin_size_in_dots;
        size_t next_block_index = block_index + (m_settings.is_forward_typing_direction ? 1 : -1);
        size_t& next_displayed_text_length = m_displayed_text_lengths[next_block_index];

        if (m_settings.is_forward_typing_direction && next_displayed_text_length == 0)
            next_displayed_text_length = 1;

        if (!m_settings.is_forward_typing_direction && next_displayed_text_length == g_text_blocks[next_block_index].length())
            next_displayed_text_length = g_text_blocks[next_block_index].length() - 1;

        return;
    }

    if (m_settings.is_forward_typing_direction)
        displayed_text_length++;
    else
        displayed_text_length--;

    const std::u32string displayed_text = full_text.substr(0, displayed_text_length);
    const gui::UnitRect  text_block_rect = GetTextBlockRectInDots(block_index, vertical_text_pos_in_dots, GetFrameSizeInDots());

    m_text_update_duration = UpdateText(text, displayed_text, text_block_rect);

    vertical_text_pos_in_dots = text.GetRectInDots().GetBottom() + g_margin_size_in_dots;
}

void TypographyApp::ResetAnimation()
{
    for(size_t block_index = 0; block_index < g_text_blocks_count; ++block_index)
    {
        const std::u32string& full_text = g_text_blocks[block_index];

        size_t displayed_text_length = block_index ? 0 : 1;
        if (!m_settings.is_forward_typing_direction)
        {
            displayed_text_length = full_text.length();
            if (block_index == g_text_blocks_count - 1)
                displayed_text_length--;
        }

        const std::u32string displayed_text = full_text.substr(0, displayed_text_length);
        m_displayed_text_lengths[block_index] = displayed_text_length;
        m_texts[block_index]->SetText(displayed_text);
        m_fonts[block_index]->ResetChars(displayed_text);
    }
    
    LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
}

bool TypographyApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    // Update text block resources
    for(const Ptr<gui::Text>& text_ptr : m_texts)
    {
        text_ptr->Update(GetFrameSize());
    }

    return true;
}

bool TypographyApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    const TypographyFrame& frame = GetCurrentFrame();

    // Draw text blocks
    META_DEBUG_GROUP_CREATE_VAR(s_text_debug_group, "Text Blocks Rendering");
    for(const Ptr<gui::Text>& text_ptr : m_texts)
    {
        text_ptr->Draw(*frame.render_cmd_list_ptr, s_text_debug_group.get());
    }

    // Draw font atlas badges
    META_DEBUG_GROUP_CREATE_VAR(s_atlas_debug_group, "Font Atlases Rendering");
    for(const Ptr<gui::Badge>& badge_atlas_ptr : m_font_atlas_badges)
    {
        badge_atlas_ptr->Draw(*frame.render_cmd_list_ptr, s_atlas_debug_group.get());
    }

    RenderOverlay(*frame.render_cmd_list_ptr);

    // Commit command list with present flag
    frame.render_cmd_list_ptr->Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandQueue().Execute(*frame.execute_cmd_list_set_ptr);
    GetRenderContext().Present();

    return true;
}

std::string TypographyApp::GetParametersString()
{
    std::stringstream ss;
    ss << "Typography parameters:"
       << std::endl << "  - text wrap mode:            " << magic_enum::enum_name(m_settings.text_layout.wrap)
       << std::endl << "  - horizontal text alignment: " << magic_enum::enum_name(m_settings.text_layout.horizontal_alignment)
       << std::endl << "  - vertical text alignment:   " << magic_enum::enum_name(m_settings.text_layout.vertical_alignment)
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
    for (const Ptr<gui::Text>& text_ptr : m_texts)
    {
        text_ptr->SetLayout(text_layout);
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
    for(const Ptr<gui::Text>& text_ptr : m_texts)
    {
        text_ptr->SetIncrementalUpdate(is_incremental_text_update);
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

void TypographyApp::OnFontAtlasTextureReset(gui::Font& font, const Ptr<gfx::Texture>& old_atlas_texture_ptr, const Ptr<gfx::Texture>& new_atlas_texture_ptr)
{
    const auto font_atlas_badge_ptr_it = std::find_if(m_font_atlas_badges.begin(), m_font_atlas_badges.end(),
                                                     [&old_atlas_texture_ptr](const Ptr<gui::Badge>& font_atlas_badge_ptr)
        {
           return std::addressof(font_atlas_badge_ptr->GetTexture()) == old_atlas_texture_ptr.get();
        }
    );

    if (new_atlas_texture_ptr)
    {
        if (font_atlas_badge_ptr_it == m_font_atlas_badges.end())
        {
            m_font_atlas_badges.emplace_back(CreateFontAtlasBadge(font, new_atlas_texture_ptr));
            LayoutFontAtlasBadges(GetRenderContext().GetSettings().frame_size);
        }
        else
        {
            const Ptr<gui::Badge>& badge_ptr = *font_atlas_badge_ptr_it;
            badge_ptr->SetTexture(new_atlas_texture_ptr);
            badge_ptr->SetSize(gui::UnitSize(gui::Units::Pixels, static_cast<const gfx::FrameSize&>(new_atlas_texture_ptr->GetSettings().dimensions)));
        }
    }
    else if (font_atlas_badge_ptr_it != m_font_atlas_badges.end())
    {
        m_font_atlas_badges.erase(font_atlas_badge_ptr_it);
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
