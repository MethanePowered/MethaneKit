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

FILE: TextRenderApp.cpp
Tutorial demonstrating triangle rendering with Methane graphics API

******************************************************************************/

#pragma once

#include <Methane/Kit.h>
#include <Methane/Data/Receiver.hpp>

#include <map>

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;

struct TextRenderFrame final : gfx::AppFrame
{
    Ptr<gfx::RenderCommandList> sp_render_cmd_list;
    Ptr<gfx::CommandListSet>    sp_execute_cmd_lists;

    using gfx::AppFrame::AppFrame;
};

using GraphicsApp = gfx::App<TextRenderFrame>;

class TextRenderApp final
    : public GraphicsApp
    , protected Data::Receiver<gfx::IFontCallback>
{
public:
    TextRenderApp();
    ~TextRenderApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool Render() override;

protected:
    // IContextCallback overrides
    void OnContextReleased(gfx::Context& context) override;

    // IFontCallback overrides
    void OnFontAtlasTextureReset(gfx::Font& font, const Ptr<gfx::Texture>& sp_old_atlas_texture, const Ptr<gfx::Texture>& sp_new_atlas_texture) override;
    void OnFontAtlasUpdated(gfx::Font& font) override;

private:
    bool Animate(double elapsed_seconds, double delta_seconds);
    void ResetAnimation();

    Ptr<gfx::Badge> CreateFontAtlasBadge(gfx::Font& font, const Ptr<gfx::Texture>& sp_atlas_texture);
    void UpdateFontAtlasBadges();
    void LayoutFontAtlasBadges(const gfx::FrameSize& frame_size);

    Ptrs<gfx::Font>     m_fonts;
    Ptrs<gfx::Text>     m_texts;
    Ptrs<gfx::Badge>    m_font_atlas_badges;
    std::vector<size_t> m_displayed_text_lengths;
    double              m_text_update_elapsed_sec = 0.0;
};

} // namespace Methane::Tutorials
