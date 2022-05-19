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

#include <string>
#include <array>

namespace Methane::Graphics
{
struct RenderPattern;
struct RenderPass;
struct RenderCommandList;
struct CommandListSet;
struct Texture;
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

    using CubeSliceDescs = std::array<TextureLabeler::SliceDesc, 6>;

    TextureLabeler(gui::Context& gui_context, const Data::Provider& font_provider, gfx::Texture& rt_texture,
                   uint32_t font_size_pt, const gfx::Color4F& text_color = { 1.F, 1.F, 1.F, 1.F },
                   const CubeSliceDescs& cube_slice_descs = {{
        { "X+", gfx::Color4F(0.84F, 0.19F, 0.17F, 1.F) }, // red       rgb(215 48 44)
        { "X-", gfx::Color4F(0.94F, 0.42F, 0.07F, 1.F) }, // orange    rgb(239 106 18)
        { "Y+", gfx::Color4F(0.35F, 0.69F, 0.24F, 1.F) }, // green     rgb(89 176 60)
        { "Y-", gfx::Color4F(0.12F, 0.62F, 0.47F, 1.F) }, // turquoise rgb(31 158 120)
        { "Z+", gfx::Color4F(0.20F, 0.36F, 0.66F, 1.F) }, // blue      rgb(51 93 169)
        { "Z-", gfx::Color4F(0.49F, 0.31F, 0.64F, 1.F) }  // purple    rgb(124 80 164)
    }});

    void Render();

private:
    struct Slice : SliceDesc
    {
        Slice(const SliceDesc& slice_desc) : SliceDesc(slice_desc) {}

        Ptr<gfx::RenderPattern>     render_pattern_ptr;
        Ptr<gfx::RenderPass>        render_pass_ptr;
        Ptr<gfx::RenderCommandList> render_cmd_list_ptr;
        Ptr<gui::Text>              label_text_ptr;
    };

    gui::Context&            m_gui_context;
    gfx::Texture&            m_rt_texture;
    gui::Font&               m_font;
    std::vector<Slice>       m_slices;
    Ptr<gfx::CommandListSet> m_slice_cmd_list_set_ptr;
};

} // namespace Methane::Tutorials
