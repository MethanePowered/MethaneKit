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

FILE: Methane/Graphics/HeadsUpDisplay.cpp
HeadsUpDisplay rendering primitive.

******************************************************************************/

#include <Methane/Graphics/HeadsUpDisplay.h>

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/Font.h>
#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <sstream>

namespace Methane::Graphics
{

HeadsUpDisplay::HeadsUpDisplay(RenderContext& context)
    : HeadsUpDisplay(context, Settings())
{
}

HeadsUpDisplay::HeadsUpDisplay(RenderContext& context, Settings settings)
    : m_settings(std::move(settings))
    , m_context(context)
    , m_sp_major_font(Font::Library::Get().GetFont(
        Data::FontProvider::Get(),
        Font::Settings
        {
            "HUD Major Font", "Fonts/RobotoMono/RobotoMono-Bold.ttf", 18u,
            context.GetFontResolutionDPI(), Font::GetAnsiCharacters()
        }
    ).GetPtr())
    , m_sp_minor_font(Font::Library::Get().GetFont(
        Data::FontProvider::Get(),
        Font::Settings
        {
            "HUD Minor Font", "Fonts/RobotoMono/RobotoMono-Regular.ttf", 12u,
            context.GetFontResolutionDPI(), Font::GetAnsiCharacters()
        }
    ).GetPtr())
    , m_fps_text(context, *m_sp_major_font,
        Text::Settings
        {
            "FPS",
            "000 FPS",
            FrameRect{ { 20, 20 }, { 500, 60 } },
            m_settings.blend_color
        }
    )
{
    ITT_FUNCTION_TASK();
}

void HeadsUpDisplay::Update()
{
    ITT_FUNCTION_TASK();

    if (m_update_timer.GetElapsedSecondsD() < m_settings.update_interval_sec)
        return;

    const FpsCounter&              fps_counter           = m_context.GetFpsCounter();
    const uint32_t                 average_fps           = fps_counter.GetFramesPerSecond();
    const FpsCounter::FrameTiming  average_frame_timing  = fps_counter.GetAverageFrameTiming();

    std::stringstream fps_ss;
    fps_ss.precision(2);
    fps_ss << average_fps
           << " FPS, " << std::fixed << average_frame_timing.GetTotalTimeMSec()
           << " ms, "  << std::fixed << average_frame_timing.GetCpuTimePercent() << "% CPU";
    m_fps_text.SetText(fps_ss.str());

    m_update_timer.Reset();
}

void HeadsUpDisplay::Draw(RenderCommandList& cmd_list) const
{
    ITT_FUNCTION_TASK();
    m_fps_text.Draw(cmd_list);
}

} // namespace Methane::Graphics
