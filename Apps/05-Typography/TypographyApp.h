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

FILE: TypographyApp.h
Tutorial demonstrating dynamic text rendering and fonts management with Methane Kit.

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/UserInterface/App.hpp>
#include <Methane/Data/Receiver.hpp>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;
namespace gui = Methane::UserInterface;

struct TypographyFrame final : gfx::AppFrame
{
    rhi::RenderCommandList render_cmd_list;
    rhi::CommandListSet    execute_cmd_list_set;

    using gfx::AppFrame::AppFrame;
};

using UserInterfaceApp = UserInterface::App<TypographyFrame>;

class TypographyApp final // NOSONAR - destructor required
    : public UserInterfaceApp
    , private Data::Receiver<gui::IFontLibraryCallback> // NOSONAR
    , private Data::Receiver<gui::IFontCallback>        // NOSONAR
{
public:
    struct Settings
    {
        gui::Text::Layout text_layout                 { gui::Text::Wrap::Word, gui::Text::HorizontalAlignment::Center, gui::Text::VerticalAlignment::Top };
        bool              is_incremental_text_update  = true;
        bool              is_forward_typing_direction = true;
        double            typing_update_interval_sec  = 0.03;
    };

    TypographyApp();
    ~TypographyApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Update() override;
    bool Render() override;

    // UserInterface::App overrides
    std::string GetParametersString() override;

    // Settings accessors
    const Settings& GetSettings() const noexcept { return m_settings; }
    void SetTextLayout(const gui::Text::Layout& text_layout);
    void SetForwardTypingDirection(bool is_forward_typing_direction);
    void SetTextUpdateInterval(double text_update_interval_sec);
    void SetIncrementalTextUpdate(bool is_incremental_text_update);

private:
    // IContextCallback overrides
    void OnContextReleased(rhi::IContext& context) override;

    // IFontLibraryCallback implementation
    void OnFontAdded(gui::Font& font) override;
    void OnFontRemoved(gui::Font&) override { /* not handled in this controller */ }

    // IFontCallback implementation
    void OnFontAtlasTextureReset(gui::Font& font, const rhi::Texture* old_atlas_texture_ptr, const rhi::Texture* new_atlas_texture_ptr) override;
    void OnFontAtlasUpdated(gui::Font& font) override;

    bool Animate(double elapsed_seconds, double);
    void AnimateTextBlock(size_t block_index, int32_t& vertical_text_pos_in_dots);
    void ResetAnimation();

    Ptr<gui::Badge> CreateFontAtlasBadge(const gui::Font& font, const rhi::Texture& atlas_texture);
    void UpdateFontAtlasBadges();
    void LayoutFontAtlasBadges(const gfx::FrameSize& frame_size);

    Settings               m_settings;
    gui::FontContext       m_font_context;
    std::vector<gui::Font> m_fonts;
    Ptrs<gui::TextItem>    m_texts;
    Ptrs<gui::Badge>       m_font_atlas_badges;
    std::vector<size_t>    m_displayed_text_lengths;
    double                 m_text_update_elapsed_sec = 0.0;
    Timer::TimeDuration    m_text_update_duration;
};

} // namespace Methane::Tutorials
