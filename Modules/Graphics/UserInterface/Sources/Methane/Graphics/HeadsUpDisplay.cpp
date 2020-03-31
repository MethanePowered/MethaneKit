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
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

HeadsUpDisplay::HeadsUpDisplay(RenderContext& context)
    : HeadsUpDisplay(context, Settings())
{
}

HeadsUpDisplay::HeadsUpDisplay(RenderContext& context, Settings settings)
    : m_settings(std::move(settings))
    , m_sp_major_font(Font::Library::Get().Add(
        Data::FontProvider::Get(),
        Font::Settings
        {
            "HUD Major Font", "Fonts/RobotoMono/RobotoMono-Bold.ttf", 18u,
            context.GetFontResolutionDPI(), Font::GetAnsiCharacters()
        }
    ))
    , m_sp_minor_font(Font::Library::Get().Add(
        Data::FontProvider::Get(),
        Font::Settings
        {
            "HUD Minor Font", "Fonts/RobotoMono/RobotoMono-Regular.ttf", 12u,
            context.GetFontResolutionDPI(), Font::GetAnsiCharacters()
        }
    ))
    , m_fps_text(context, *m_sp_major_font,
        Text::Settings
        {
            "FPS",
            "0 FPS",
            FrameRect{ { 20, 20 }, { 100, 60 } },
            m_settings.blend_color
        }
    )
{
    ITT_FUNCTION_TASK();
}

void HeadsUpDisplay::Update() const
{
    ITT_FUNCTION_TASK();
}

void HeadsUpDisplay::Draw(RenderCommandList& cmd_list) const
{
    ITT_FUNCTION_TASK();
    m_fps_text.Draw(cmd_list);
}

} // namespace Methane::Graphics
