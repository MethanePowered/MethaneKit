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

#include <Methane/Graphics/Kit.h>

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

class TextRenderApp final : public GraphicsApp
{
public:
    TextRenderApp();
    ~TextRenderApp() override;

    // GraphicsApp overrides
    void Init() override;
    bool Resize(const gfx::FrameSize& frame_size, bool is_minimized) override;
    bool UpdateText(double elapsed_seconds, double delta_seconds);
    bool Render() override;

    // Context::Callback interface
    void OnContextReleased(gfx::Context& context) override;

private:
    void InitFontAtlasBadges();
    void LayoutFontAtlasBadges(const gfx::FrameSize& frame_size);

    Ptr<gfx::Font>   m_sp_primary_font;
    Ptr<gfx::Font>   m_sp_secondary_font;
    Ptr<gfx::Text>   m_sp_primary_text;
    Ptr<gfx::Text>   m_sp_secondary_text;
    Ptrs<gfx::Badge> m_sp_font_atlas_badges;

    double m_text_update_elapsed_sec         = 0.0;
    size_t m_secondary_text_displayed_length = 1;
};

} // namespace Methane::Tutorials
