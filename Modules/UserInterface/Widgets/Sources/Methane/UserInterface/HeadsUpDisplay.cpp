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
HeadsUpDisplay rendering primitive.

******************************************************************************/

#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/UserInterface/Context.h>

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <sstream>

namespace Methane::UserInterface
{

HeadsUpDisplay::HeadsUpDisplay(Context& ui_context, const Data::Provider& font_data_provider, Settings settings)
    : Item(ui_context)
    , m_settings(std::move(settings))
    , m_sp_major_font(Font::Library::Get().GetFont(
        font_data_provider, Font::Settings{ m_settings.major_font, GetUIContext().GetFontResolutionDpi(), Font::GetAlphabetDefault() }
    ).GetPtr())
    , m_fps_text(ui_context, *m_sp_major_font,
        Text::SettingsUtf8
        {
            "FPS",
            "000 FPS",
            UnitRect(settings.position, { }, settings.position.units),
            Text::Layout{ },
            m_settings.text_color
        }
    )
{
    META_FUNCTION_TASK();
}

void HeadsUpDisplay::SetTextColor(const gfx::Color4f& text_color)
{
    META_FUNCTION_TASK();
    if (m_settings.text_color == text_color)
        return;

    m_settings.text_color = text_color;
    m_fps_text.SetColor(text_color);
}

void HeadsUpDisplay::SetUpdateInterval(double update_interval_sec)
{
    META_FUNCTION_TASK();
    m_settings.update_interval_sec = update_interval_sec;
}

bool HeadsUpDisplay::SetRect(const UnitRect& rect)
{
    META_FUNCTION_TASK();
    if (!Item::SetRect(rect))
        return false;

    m_fps_text.SetRect(rect);
    return true;
}

void HeadsUpDisplay::Update()
{
    META_FUNCTION_TASK();
    if (m_update_timer.GetElapsedSecondsD() < m_settings.update_interval_sec)
        return;

    const gfx::FpsCounter&              fps_counter           = GetUIContext().GetRenderContext().GetFpsCounter();
    const uint32_t                      average_fps           = fps_counter.GetFramesPerSecond();
    const gfx::FpsCounter::FrameTiming  average_frame_timing  = fps_counter.GetAverageFrameTiming();

    std::stringstream fps_ss;
    fps_ss.precision(2);
    fps_ss << average_fps
           << " FPS, " << std::fixed << average_frame_timing.GetTotalTimeMSec()
           << " ms, "  << std::fixed << average_frame_timing.GetCpuTimePercent() << "% CPU";
    m_fps_text.SetText(fps_ss.str());

    m_update_timer.Reset();
}

void HeadsUpDisplay::Draw(gfx::RenderCommandList& cmd_list)
{
    META_FUNCTION_TASK();
    m_fps_text.Draw(cmd_list);
}

} // namespace Methane::UserInterface
