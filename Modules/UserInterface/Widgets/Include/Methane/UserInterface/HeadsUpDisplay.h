/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/HeadsUpDisplay.h
Heads-Up-Display widget for displaying runtime rendering parameters.

******************************************************************************/

#pragma once

#include <Methane/UserInterface/Panel.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Graphics/RHI/IRenderCommandList.h>
#include <Methane/Platform/Keyboard.h>
#include <Methane/Timer.hpp>
#include <Methane/Memory.hpp>

namespace Methane::UserInterface
{

class Font;

class HeadsUpDisplay final : public Panel
{
public:
    struct Settings
    {
        Font::Description         major_font          { "Major", "Fonts/RobotoMono/RobotoMono-Bold.ttf",    24U };
        Font::Description         minor_font          { "Minor", "Fonts/RobotoMono/RobotoMono-Regular.ttf", 9U };
        UnitPoint                 position            { Units::Dots, 20, 20 };
        UnitSize                  text_margins        { Units::Dots, 16, 8  };
        Color4F                   text_color          { 1.F,  1.F,  1.F,  1.F   };
        Color4F                   on_color            { 0.3F, 1.F,  0.3F, 1.F   };
        Color4F                   off_color           { 1.F,  0.3F, 0.3F, 1.F   };
        Color4F                   help_color          { 1.F,  1.F,  0.0F, 1.F   };
        Color4F                   background_color    { 0.F,  0.F,  0.F,  0.66F };
        Platform::Keyboard::State help_shortcut       { Platform::Keyboard::Key::F1 };
        double                    update_interval_sec = 0.33;

        Settings& SetMajorFont(const Font::Description& new_major_font) noexcept;
        Settings& SetMinorFont(const Font::Description& new_minor_font) noexcept;
        Settings& SetPosition(const UnitPoint& new_position) noexcept;
        Settings& SetTextMargins(const UnitSize& new_text_margins) noexcept;
        Settings& SetTextColor(const Color4F& new_text_color) noexcept;
        Settings& SetOnColor(const Color4F& new_on_color) noexcept;
        Settings& SetOffColor(const Color4F& new_off_color) noexcept;
        Settings& SetHelpColor(const Color4F& new_help_color) noexcept;
        Settings& SetBackgroundColor(const Color4F& new_background_color) noexcept;
        Settings& SetHelpShortcut(const Platform::Keyboard::State& new_help_shortcut) noexcept;
        Settings& SetUpdateIntervalSec(double new_update_interval_sec) noexcept;
    };

    HeadsUpDisplay(Context& ui_context, const Data::IProvider& font_data_provider, const Settings& settings);

    const Settings& GetHudSettings() const { return m_settings; }

    void SetTextColor(const Color4F& text_color);
    void SetUpdateInterval(double update_interval_sec);

    void Update(const FrameSize& render_attachment_size);
    void Draw(rhi::IRenderCommandList& cmd_list, rhi::ICommandListDebugGroup* p_debug_group = nullptr) const override;

private:
    enum class TextBlock : size_t
    {
        Fps = 0U,
        FrameTime,
        CpuTime,
        GpuName,
        HelpKey,
        FrameBuffersAndApi,
        VSync,

        Count
    };

    using TextBlockPtrs = std::array<Ptr<Text>, static_cast<size_t>(TextBlock::Count)>;
    Text& GetTextBlock(TextBlock block) const;

    void LayoutTextBlocks();
    void UpdateAllTextBlocks(const FrameSize& render_attachment_size) const;

    Settings            m_settings;
    const Ptr<Font>     m_major_font_ptr;
    const Ptr<Font>     m_minor_font_ptr;
    const TextBlockPtrs m_text_blocks;
    Timer               m_update_timer;
};

} // namespace Methane::UserInterface
