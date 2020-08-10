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

FILE: Methane/UserInterface/HeadsUpDisplay.cpp
Heads-Up-Display widget for displaying graphics application runtime parameters.

 ╔═══════════════════════════╤══════════════╗
 ║ GPU Name                  ┆ F1 - Help    ║
 ╟┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┼┈┈┈┈┈┈┈┈┈┈┈┈┈┈╢
 ║                           ┆ Frame Time   ║
 ║   123 FPS (Major Font)    ├┈┈┈┈┈┈┈┈┈┈┈┈┈┈╢
 ║                           ┆ CPU Time %   ║
 ╟┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┼┈┈┈┈┈┈┈┈┈┈┈┈┈┈╢
 ║ Frame Buffers Res & Count ┆ VSync ON/OFF ║
 ╚═══════════════════════════╧══════════════╝

******************************************************************************/

#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/UserInterface/Context.h>

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Graphics/Device.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <sstream>

namespace Methane::UserInterface
{

inline uint32_t GetFpsTextHeightInDots(Context& ui_context, Font& major_font, Font& minor_font, const UnitSize& text_margins)
{
    return std::max(ui_context.ConvertPixelsToDots(major_font.GetLineHeight()),
                    ui_context.ConvertPixelsToDots(minor_font.GetLineHeight()) * 2u + ui_context.ConvertToDots(text_margins).height);
}

inline uint32_t GetTimingTextHeightInDots(Context& ui_context, Font& major_font, Font& minor_font, const UnitSize& text_margins)
{
    return (GetFpsTextHeightInDots(ui_context, major_font, minor_font, text_margins) - ui_context.ConvertToDots(text_margins).height) / 2u;
}

template <typename T>
std::string ToStringWithPrecision(const T value, const int precision)
{
    std::ostringstream ss;
    ss.precision(precision);
    ss << std::fixed << value;
    return ss.str();
}

HeadsUpDisplay::HeadsUpDisplay(Context& ui_context, const Data::Provider& font_data_provider, Settings settings)
    : Panel(ui_context, { }, { "Heads Up Display" })
    , m_settings(std::move(settings))
    , m_sp_major_font(
        Font::Library::Get().GetFont(font_data_provider,
            Font::Settings
            {
                m_settings.major_font,
                GetUIContext().GetFontResolutionDpi(),
                Font::GetAlphabetDefault()
            }
        ).GetPtr()
    )
    , m_sp_minor_font(
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
        std::make_shared<Text>(ui_context, *m_sp_major_font,
            Text::SettingsUtf8
            {
                "FPS",
                "000 FPS",
                UnitRect{ { }, { 0u, GetFpsTextHeightInDots(ui_context, *m_sp_major_font, *m_sp_minor_font, m_settings.text_margins) }, Units::Dots },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_sp_minor_font,
            Text::SettingsUtf8
            {
                "Frame Time",
                "00.00 ms",
                UnitRect{ { }, { 0u, GetTimingTextHeightInDots(ui_context, *m_sp_major_font, *m_sp_minor_font, m_settings.text_margins) }, Units::Dots },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_sp_minor_font,
            Text::SettingsUtf8
            {
                "CPU Time",
                "00.00% cpu",
                UnitRect{ { }, { 0u, GetTimingTextHeightInDots(ui_context, *m_sp_major_font, *m_sp_minor_font, m_settings.text_margins) }, Units::Dots },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_sp_minor_font,
            Text::SettingsUtf8
            {
                "GPU",
                "Graphics Adapter",
                UnitRect{ },
                Text::Layout{ Text::Wrap::None },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_sp_minor_font,
            Text::SettingsUtf8
            {
                "Help",
                "F1 - Help",
                UnitRect{ },
                Text::Layout{ Text::Wrap::None },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_sp_minor_font,
            Text::SettingsUtf8
            {
                "Frame Buffers",
                "0000 x 0000   3 FB",
                UnitRect{ },
                Text::Layout{ Text::Wrap::None },
                m_settings.text_color
            }
        ),
        std::make_shared<Text>(ui_context, *m_sp_minor_font,
            Text::SettingsUtf8
            {
                "VSync",
                "VSync ON",
                UnitRect{ },
                Text::Layout{ Text::Wrap::None },
                m_settings.text_color
            }
        )
    })
{
    META_FUNCTION_TASK();
    for(const Ptr<Text>& sp_text : m_text_blocks)
    {
        AddChild(*sp_text);
    }
    LayoutTextBlocks();
}

void HeadsUpDisplay::SetTextColor(const gfx::Color4f& text_color)
{
    META_FUNCTION_TASK();
    if (m_settings.text_color == text_color)
        return;

    m_settings.text_color = text_color;

    for(const Ptr<Text>& sp_text : m_text_blocks)
    {
        sp_text->SetColor(text_color);
    }
}

void HeadsUpDisplay::SetUpdateInterval(double update_interval_sec)
{
    META_FUNCTION_TASK();
    m_settings.update_interval_sec = update_interval_sec;
}

void HeadsUpDisplay::Update()
{
    META_FUNCTION_TASK();
    if (m_update_timer.GetElapsedSecondsD() < m_settings.update_interval_sec)
    {
        UpdateAllTextBlocks();
        return;
    }

    const gfx::FpsCounter& fps_counter = GetUIContext().GetRenderContext().GetFpsCounter();
    const gfx::RenderContext::Settings& context_settings = GetUIContext().GetRenderContext().GetSettings();

    std::stringstream frame_buffers_ss;
    frame_buffers_ss << context_settings.frame_size.width << " x " << context_settings.frame_size.height << "    " << context_settings.frame_buffers_count << " FB";

    m_text_blocks[TextBlock::Fps]->SetText(std::to_string(fps_counter.GetFramesPerSecond()) + " FPS");
    m_text_blocks[TextBlock::FrameTime]->SetText(ToStringWithPrecision(fps_counter.GetAverageFrameTiming().GetTotalTimeMSec(), 2) + " ms");
    m_text_blocks[TextBlock::CpuTime]->SetText(ToStringWithPrecision(fps_counter.GetAverageFrameTiming().GetCpuTimePercent(), 2) + "% cpu");
    m_text_blocks[TextBlock::GpuName]->SetText(GetUIContext().GetRenderContext().GetDevice().GetAdapterName());
    m_text_blocks[TextBlock::FrameBuffers]->SetText(frame_buffers_ss.str());
    m_text_blocks[TextBlock::VSync]->SetText(context_settings.vsync_enabled ? "VSync ON" : "VSync OFF");

    LayoutTextBlocks();
    UpdateAllTextBlocks();
    m_update_timer.Reset();
}

void HeadsUpDisplay::Draw(gfx::RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    Panel::Draw(cmd_list);

    for(const Ptr<Text>& sp_text : m_text_blocks)
    {
        sp_text->Draw(cmd_list);
    }
}

void HeadsUpDisplay::LayoutTextBlocks()
{
    META_FUNCTION_TASK();
    const UnitSize text_margins_in_dots = GetUIContext().ConvertToDots(m_settings.text_margins);

    // Layout Left column text blocks
    const UnitSize gpu_name_size      = m_text_blocks[TextBlock::GpuName]->GetRectInDots().GetUnitSize();
    const UnitSize fps_size           = m_text_blocks[TextBlock::Fps]->GetRectInDots().GetUnitSize();
    const UnitSize frame_buffers_size = m_text_blocks[TextBlock::FrameBuffers]->GetRectInDots().GetUnitSize();
    const uint32_t left_column_width  = std::max({ gpu_name_size.width, fps_size.width, frame_buffers_size.width });

    UnitPoint position(text_margins_in_dots.width, text_margins_in_dots.height, Units::Dots);
    m_text_blocks[TextBlock::GpuName]->SetRelOrigin(position);

    position.SetY(position.GetY() + gpu_name_size.height + text_margins_in_dots.height);
    m_text_blocks[TextBlock::Fps]->SetRelOrigin(position);

    position.SetY(position.GetY() + fps_size.height + text_margins_in_dots.height);
    m_text_blocks[TextBlock::FrameBuffers]->SetRelOrigin(position);

    // LayoutRight column text block sizes
    const UnitSize help_size          = m_text_blocks[TextBlock::HelpKey]->GetRectInDots().GetUnitSize();
    const UnitSize frame_time_size    = m_text_blocks[TextBlock::FrameTime]->GetRectInDots().GetUnitSize();
    const UnitSize cpu_time_size      = m_text_blocks[TextBlock::CpuTime]->GetRectInDots().GetUnitSize();
    const UnitSize vsync_size         = m_text_blocks[TextBlock::VSync]->GetRectInDots().GetUnitSize();
    const uint32_t right_column_width = std::max({ help_size.width, frame_time_size.width, cpu_time_size.width, vsync_size.width });

    // Layout right column
    position = UnitPoint(left_column_width + 2 * text_margins_in_dots.width, text_margins_in_dots.height, Units::Dots);
    m_text_blocks[TextBlock::HelpKey]->SetRelOrigin(position);

    position.SetY(position.GetY() + help_size.height + text_margins_in_dots.height);
    m_text_blocks[TextBlock::FrameTime]->SetRelOrigin(position);

    position.SetY(position.GetY() + frame_time_size.height + text_margins_in_dots.height);
    m_text_blocks[TextBlock::CpuTime]->SetRelOrigin(position);

    position.SetY(position.GetY() + cpu_time_size.height + text_margins_in_dots.height);
    m_text_blocks[TextBlock::VSync]->SetRelOrigin(position);

    Panel::SetRect(UnitRect{
        m_settings.position,
        {
            position.GetX() + right_column_width + text_margins_in_dots.width,
            position.GetY() + vsync_size.height  + text_margins_in_dots.height
        },
        Units::Dots
    });
}

void HeadsUpDisplay::UpdateAllTextBlocks()
{
    META_FUNCTION_TASK();
    for(const Ptr<Text>& sp_text : m_text_blocks)
    {
        sp_text->Update();
    }
}

} // namespace Methane::UserInterface
