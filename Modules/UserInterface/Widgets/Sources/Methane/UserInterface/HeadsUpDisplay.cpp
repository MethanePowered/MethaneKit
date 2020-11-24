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

FILE: Methane/UserInterface/HeadsUpDisplay.cpp
Heads-Up-Display widget for displaying runtime rendering parameters.

 ╔═══════════════╤════════════════════════════════╗
 ║ F1 - Help     │ GPU Adapter Name               ║
 ╟───────────────┼────────────────────────────────╢
 ║ Frame Time ms │                                ║
 ╟───────────────┥ 123 FPS (Major Font)           ║
 ║ CPU Time %    │                                ║
 ╟───────────────┼────────────────────────────────╢
 ║ VSync ON/OFF  │ Frame Buffs Resolution & Count ║
 ╚═══════════════╧════════════════════════════════╝

******************************************************************************/

#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/UserInterface/Context.h>

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::UserInterface
{

static constexpr uint32_t g_first_line_height_decrement = 5;

inline uint32_t GetTextHeightInDots(Context& ui_context, Font& font)
{
    return ui_context.ConvertPixelsToDots(font.GetMaxGlyphSize().height);
}

inline uint32_t GetFpsTextHeightInDots(Context& ui_context, Font& major_font, Font& minor_font, const UnitSize& text_margins)
{
    return std::max(GetTextHeightInDots(ui_context, major_font), GetTextHeightInDots(ui_context, minor_font) * 2U + ui_context.ConvertToDots(text_margins).height);
}

inline uint32_t GetTimingTextHeightInDots(Context& ui_context, Font& major_font, Font& minor_font, const UnitSize& text_margins)
{
    return (GetFpsTextHeightInDots(ui_context, major_font, minor_font, text_margins) - ui_context.ConvertToDots(text_margins).height) / 2U;
}

HeadsUpDisplay::HeadsUpDisplay(Context& ui_context, const Data::Provider& font_data_provider, Settings settings)
    : Panel(ui_context, { }, { "Heads Up Display" })
    , m_settings(std::move(settings))
    , m_major_font_ptr(
        Font::Library::Get().GetFont(font_data_provider,
            Font::Settings
            {
                m_settings.major_font,
                GetUIContext().GetFontResolutionDpi(),
                U"FPS0123456789",
            }
        ).GetPtr()
    )
    , m_minor_font_ptr(
        Font::Library::Get().GetFont(font_data_provider,
            Font::Settings
            {
                m_settings.minor_font,
                GetUIContext().GetFontResolutionDpi(),
                Font::GetAlphabetDefault()
            }
        ).GetPtr()
    )
    , m_text_blocks({
        std::make_shared<Text>(ui_context, *m_major_font_ptr,
            Text::SettingsUtf8
            {
                "FPS",
                "000 FPS",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetFpsTextHeightInDots(ui_context, *m_major_font_ptr, *m_minor_font_ptr, m_settings.text_margins) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_minor_font_ptr,
            Text::SettingsUtf8
            {
                "Frame Time",
                "00.00 ms",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetTimingTextHeightInDots(ui_context, *m_major_font_ptr, *m_minor_font_ptr, m_settings.text_margins) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_minor_font_ptr,
            Text::SettingsUtf8
            {
                "CPU Time",
                "00.00% cpu",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetTimingTextHeightInDots(ui_context, *m_major_font_ptr, *m_minor_font_ptr, m_settings.text_margins) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_minor_font_ptr,
            Text::SettingsUtf8
            {
                "GPU",
                "Graphics Adapter",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, *m_minor_font_ptr) - g_first_line_height_decrement } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_minor_font_ptr,
            Text::SettingsUtf8
            {
                "Help",
                m_settings.help_shortcut ? m_settings.help_shortcut.ToString() + " - Help" : "",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, *m_minor_font_ptr) - g_first_line_height_decrement } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.help_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_minor_font_ptr,
            Text::SettingsUtf8
            {
                "Frame Buffers",
                "0000 x 0000   3 FB",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, *m_minor_font_ptr) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_minor_font_ptr,
            Text::SettingsUtf8
            {
                "VSync",
                "VSync ON",
                UnitRect{ Units::Dots, gfx::Point2i{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, *m_minor_font_ptr) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.on_color
            }
        )
    })
{
    META_FUNCTION_TASK();

    // Add HUD text blocks as children to the base panel container
    for(const Ptr<Text>& text_ptr : m_text_blocks)
    {
        AddChild(*text_ptr);
    }

    // Reset timer behind so that HUD is filled with actual values on first update
    m_update_timer.ResetToSeconds(m_settings.update_interval_sec);
}

void HeadsUpDisplay::SetTextColor(const gfx::Color4f& text_color)
{
    META_FUNCTION_TASK();
    if (m_settings.text_color == text_color)
        return;

    m_settings.text_color = text_color;

    for(const Ptr<Text>& text_ptr : m_text_blocks)
    {
        text_ptr->SetColor(text_color);
    }
}

void HeadsUpDisplay::SetUpdateInterval(double update_interval_sec)
{
    META_FUNCTION_TASK();
    m_settings.update_interval_sec = update_interval_sec;
}

void HeadsUpDisplay::Update(const FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (m_update_timer.GetElapsedSecondsD() < m_settings.update_interval_sec)
    {
        UpdateAllTextBlocks(render_attachment_size);
        return;
    }

    const gfx::FpsCounter& fps_counter = GetUIContext().GetRenderContext().GetFpsCounter();
    const gfx::RenderContext::Settings& context_settings = GetUIContext().GetRenderContext().GetSettings();

    GetTextBlock(TextBlock::Fps).SetText(fmt::format("{:d} FPS", fps_counter.GetFramesPerSecond()));
    GetTextBlock(TextBlock::FrameTime).SetText(fmt::format("{:.2f} ms", fps_counter.GetAverageFrameTiming().GetTotalTimeMSec()));
    GetTextBlock(TextBlock::CpuTime).SetText(fmt::format("{:.2f}% cpu", fps_counter.GetAverageFrameTiming().GetCpuTimePercent()));
    GetTextBlock(TextBlock::GpuName).SetText(GetUIContext().GetRenderContext().GetDevice().GetAdapterName());
    GetTextBlock(TextBlock::FrameBuffers).SetText(fmt::format("{:d} x {:d}    {:d} FB", context_settings.frame_size.width, context_settings.frame_size.height, context_settings.frame_buffers_count));
    GetTextBlock(TextBlock::VSync).SetText(context_settings.vsync_enabled ? "VSync ON" : "VSync OFF");
    GetTextBlock(TextBlock::VSync).SetColor(context_settings.vsync_enabled ? m_settings.on_color : m_settings.off_color);

    LayoutTextBlocks();
    UpdateAllTextBlocks(render_attachment_size);
    m_update_timer.Reset();
}

void HeadsUpDisplay::Draw(gfx::RenderCommandList& cmd_list, gfx::CommandList::DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    Panel::Draw(cmd_list, p_debug_group);

    for(const Ptr<Text>& text_ptr : m_text_blocks)
    {
        text_ptr->Draw(cmd_list, p_debug_group);
    }
}

Text& HeadsUpDisplay::GetTextBlock(TextBlock block) const
{
    META_FUNCTION_TASK();
    const Ptr<Text>& text_block_ptr = m_text_blocks[static_cast<size_t>(block)];
    META_CHECK_ARG_NOT_NULL(text_block_ptr);
    return *text_block_ptr;
}

void HeadsUpDisplay::LayoutTextBlocks()
{
    META_FUNCTION_TASK();
    const UnitSize text_margins_in_dots = GetUIContext().ConvertToDots(m_settings.text_margins);

    // Layout left column text blocks
    const FrameSize help_size          = GetTextBlock(TextBlock::HelpKey).GetRectInDots().size;
    const FrameSize frame_time_size    = GetTextBlock(TextBlock::FrameTime).GetRectInDots().size;
    const FrameSize cpu_time_size      = GetTextBlock(TextBlock::CpuTime).GetRectInDots().size;
    const FrameSize vsync_size         = GetTextBlock(TextBlock::VSync).GetRectInDots().size;
    const uint32_t  left_column_width  = std::max({ help_size.width, frame_time_size.width, cpu_time_size.width, vsync_size.width });

    UnitPoint position(Units::Dots, text_margins_in_dots.width, text_margins_in_dots.height);
    GetTextBlock(TextBlock::HelpKey).SetRelOrigin(position);

    position.SetY(position.GetY() + help_size.height + text_margins_in_dots.height);
    GetTextBlock(TextBlock::FrameTime).SetRelOrigin(position);

    position.SetY(position.GetY() + frame_time_size.height + text_margins_in_dots.height);
    GetTextBlock(TextBlock::CpuTime).SetRelOrigin(position);

    position.SetY(position.GetY() + cpu_time_size.height + text_margins_in_dots.height);
    GetTextBlock(TextBlock::VSync).SetRelOrigin(position);

    // Layout right column text blocks
    const FrameSize gpu_name_size      = GetTextBlock(TextBlock::GpuName).GetRectInDots().size;
    const FrameSize fps_size           = GetTextBlock(TextBlock::Fps).GetRectInDots().size;
    const FrameSize frame_buffers_size = GetTextBlock(TextBlock::FrameBuffers).GetRectInDots().size;
    const uint32_t  right_column_width = std::max({ gpu_name_size.width, fps_size.width, frame_buffers_size.width });

    position.SetX(left_column_width + 2 * text_margins_in_dots.width);
    GetTextBlock(TextBlock::FrameBuffers).SetRelOrigin(position);

    const UnitPoint right_bottom_position = position;

    position.SetY(text_margins_in_dots.height);
    GetTextBlock(TextBlock::GpuName).SetRelOrigin(position);

    position.SetY(position.GetY() + gpu_name_size.height + text_margins_in_dots.height);
    GetTextBlock(TextBlock::Fps).SetRelOrigin(position);

    Panel::SetRect(UnitRect{
        Units::Dots,
        m_settings.position,
        gfx::FrameSize
        {
            right_bottom_position.GetX() + right_column_width + text_margins_in_dots.width,
            right_bottom_position.GetY() + vsync_size.height + text_margins_in_dots.height
        }
    });
}

void HeadsUpDisplay::UpdateAllTextBlocks(const FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    for(const Ptr<Text>& text_ptr : m_text_blocks)
    {
        text_ptr->Update(render_attachment_size);
    }
}

} // namespace Methane::UserInterface
