/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: TextureLabeler.h
Renders text labels to the faces of cube-map texture array

******************************************************************************/

#pragma once

#include <Methane/Graphics/Color.hpp>
#include <Methane/Graphics/IResourceBarriers.h>

#include <string>
#include <array>

namespace Methane::Graphics
{
struct RenderPattern;
struct RenderPass;
struct RenderCommandList;
struct CommandListSet;
struct ITexture;
class  ResourceBarriersBase;
class  ScreenQuad;
}

namespace Methane::UserInterface
{
class Context;
class Font;
class Text;
}

namespace Methane::Data
{
struct Provider;
}

namespace Methane::Tutorials
{

namespace gfx = Methane::Graphics;
namespace gui = Methane::UserInterface;

class TextureLabeler
{
public:
    struct SliceDesc
    {
        std::string  label;
        gfx::Color4F color;
    };

    using CubeSliceDescs = std::vector<SliceDesc>; // 6 cube faces + extra colors

    struct Settings
    {
        uint32_t       font_size_pt     = 16U;
        uint32_t       border_width_px  = 0U;
        gfx::Color4F   text_color       = { 1.F, 1.F, 1.F, 1.F };
        gfx::Color4F   border_color     = { 1.F, 1.F, 1.F, 1.F };
        CubeSliceDescs cube_slice_descs = {{
            { "X+", gfx::Color4F(0.84F, 0.19F, 0.17F, 1.F) }, // red       rgb(215 48 44)
            { "X-", gfx::Color4F(0.94F, 0.42F, 0.07F, 1.F) }, // orange    rgb(239 106 18)
            { "Y+", gfx::Color4F(0.35F, 0.69F, 0.24F, 1.F) }, // green     rgb(89 176 60)
            { "Y-", gfx::Color4F(0.12F, 0.62F, 0.47F, 1.F) }, // turquoise rgb(31 158 120)
            { "Z+", gfx::Color4F(0.20F, 0.36F, 0.66F, 1.F) }, // blue      rgb(51 93 169)
            { "Z-", gfx::Color4F(0.49F, 0.31F, 0.64F, 1.F) }, // purple    rgb(124 80 164)
            { "",   gfx::Color4F(0.90F, 0.73F, 0.00F, 1.F) }, // yellow    rgb(231, 187, 0)
            { "",   gfx::Color4F(0.00F, 0.61F, 0.75F, 1.F) }, // cyan      rgb(0, 156, 191)
            { "",   gfx::Color4F(0.93F, 0.37F, 0.66F, 1.F) }, // pink      rgb(237, 95, 169)
            { "",   gfx::Color4F(0.82F, 0.75F, 0.62F, 1.F) }, // latte     rgb(210, 191, 158)
            { "",   gfx::Color4F(0.65F, 0.63F, 0.85F, 1.F) }, // lavender  rgb(165, 160, 216)
            { "",   gfx::Color4F(0.63F, 0.84F, 0.64F, 1.F) }, // spearmint rgb(160, 214, 164)
            { "",   gfx::Color4F(0.51F, 0.71F, 0.00F, 1.F) }, // celery    rgb(130, 180, 0)
            { "",   gfx::Color4F(0.65F, 0.53F, 0.50F, 1.F) }, // rosewood  rgb(165, 135, 127)
            { "",   gfx::Color4F(0.54F, 0.65F, 0.83F, 1.F) }, // dusk      rgb(137, 165, 211)
            { "",   gfx::Color4F(0.44F, 0.23F, 0.45F, 1.F) }, // amethyst  rgb(113, 58, 116)
        }};
    };

    TextureLabeler(gui::Context& gui_context, const Data::Provider& font_provider,
                   gfx::ITexture& rt_texture, gfx::ResourceState rt_texture_final_state, const Settings& settings);

    void Render();

private:
    struct Slice : SliceDesc
    {
        explicit Slice(const SliceDesc& slice_desc)
            : SliceDesc(slice_desc)
        {}

        Ptr<gfx::RenderPass>        render_pass_ptr;
        Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
        Ptr<gui::Text>              label_text_ptr;
        Ptr<gfx::ScreenQuad>        bg_quad_ptr;
    };

    gui::Context &              m_gui_context;
    gfx::ITexture&              m_rt_texture;
    gui::Font&                  m_font;
    std::vector<Slice>          m_slices;
    Ptr<gfx::RenderPattern>     m_texture_face_render_pattern_ptr;
    Ptr<gfx::IResourceBarriers> m_ending_resource_barriers_ptr;
    Ptr<gfx::RenderPattern>     m_ending_render_pattern_ptr;
    Ptr<gfx::RenderPass>        m_ending_render_pass_ptr;
    Ptr<gfx::RenderCommandList> m_ending_render_cmd_list_ptr;
    Ptr<gfx::CommandListSet>    m_render_cmd_list_set_ptr;
};

} // namespace Methane::Tutorials
