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

FILE: Methane/UserInterface/HeadsUpDisplay.h
Heads-Up-Display widget for displaying runtime rendering parameters.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Panel.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Platform/Keyboard.h>
#include <Methane/Timer.hpp>
#include <Methane/Memory.hpp>

namespace Methane::Graphics
{
struct RenderCommandList;
}

namespace Methane::UserInterface
{

class Font;

class HeadsUpDisplay final : public Panel
{
public:
    struct Settings
    {
        Font::Description         major_font          { "Major", "Fonts/RobotoMono/RobotoMono-Bold.ttf",    24u };
        Font::Description         minor_font          { "Minor", "Fonts/RobotoMono/RobotoMono-Regular.ttf", 9u };
        UnitPoint                 position            { 20, 20, Units::Dots };
        UnitSize                  text_margins        { 16, 8,  Units::Dots };
        Color4f                   text_color          { 1.f,  1.f,  1.f,  1.f   };
        Color4f                   on_color            { 0.3f, 1.f,  0.3f, 1.f   };
        Color4f                   off_color           { 1.f,  0.3f, 0.3f, 1.f   };
        Color4f                   help_color          { 1.f,  1.f,  0.0f, 1.f   };
        Color4f                   background_color    { 0.f,  0.f,  0.f,  0.66f };
        Platform::Keyboard::State help_shortcut       { Platform::Keyboard::Key::F1 };
        double                    update_interval_sec = 0.33;
    };

    HeadsUpDisplay(Context& ui_context, const Data::Provider& font_data_provider, Settings settings);

    const Settings& GetSettings() const { return m_settings; }

    void SetTextColor(const Color4f& text_color);
    void SetUpdateInterval(double update_interval_sec);

    void Update();
    void Draw(gfx::RenderCommandList& cmd_list);

private:
    void LayoutTextBlocks();
    void UpdateAllTextBlocks();

    enum TextBlock : size_t
    {
        Fps = 0u,
        FrameTime,
        CpuTime,
        GpuName,
        HelpKey,
        FrameBuffers,
        VSync,

        Count
    };

    using TextBlockPtrs = std::array<Ptr<Text>, TextBlock::Count>;

    Settings            m_settings;
    const Ptr<Font>     m_sp_major_font;
    const Ptr<Font>     m_sp_minor_font;
    const TextBlockPtrs m_text_blocks;
    Timer               m_update_timer;
};

} // namespace Methane::UserInterface
