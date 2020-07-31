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

FILE: TypographyApp.h
Tutorial demonstrating dynamic text rendering and fonts management with Methane Kit.

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/Data/Receiver.hpp>

#include <map>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace gui = Methane::UserInterface;

struct TypographyFrame final : gfx::AppFrame
{
    Ptr<gfx::RenderCommandList> sp_render_cmd_list;
    Ptr<gfx::CommandListSet>    sp_execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<TypographyFrame>;

class TypographyApp final
    : public UserInterfaceApp
    , protected Data::Receiver<gui::IFontLibraryCallback>
    , protected Data::Receiver<gui::IFontCallback>
{
public:
    struct Settings
    {
        gui::Text::Layout text_layout                 { gui::Text::Wrap::Word };
        bool              is_incremental_text_update  = true;
        bool              is_forward_typing_direction = true;
        double            typing_update_interval_sec  = 0.03;
    };

    TypographyApp();
    ~TypographyApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Render() override;

    // UserInterface::App overrides
    std::string GetParametersString() override;

    void SetTextLayout(const gui::Text::Layout& text_layout);
    void SetForwardTypingDirection(bool is_forward_typing_direction);
    void SetTextUpdateInterval(double text_update_interval_sec);
    void SetIncrementalTextUpdate(bool is_incremental_text_update);

    const Settings& GetSettings() const noexcept { return m_settings; }

protected:
    // IContextCallback overrides
    void OnContextReleased(gfx::Context& context) override;

    // IFontLibraryCallback implementation
    void OnFontAdded(gui::Font& font) override;
    void OnFontRemoved(gui::Font&) override { }

    // IFontCallback implementation
    void OnFontAtlasTextureReset(gui::Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture) override;
    void OnFontAtlasUpdated(gui::Font& font) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);
    void ResetAnimation();

    Ptr<gui::Badge> CreateFontAtlasBadge(gui::Font& font, const Ptr<gfx::Texture>& sp_atlas_texture);
    void UpdateFontAtlasBadges();
    void LayoutFontAtlasBadges(const gfx::FrameSize& frame_size);

    Settings            m_settings;
    Ptrs<gui::Font>     m_fonts;
    Ptrs<gui::Text>     m_texts;
    Ptrs<gui::Badge>    m_font_atlas_badges;
    std::vector<size_t> m_displayed_text_lengths;
    double              m_text_update_elapsed_sec = 0.0;
    Timer::TimeDuration m_text_update_duration;
};

} // namespace Methane::Tutorials
