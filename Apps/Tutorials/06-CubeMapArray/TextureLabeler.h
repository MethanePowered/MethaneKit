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

    TextureLabeler(gui::Context& gui_context, const Data::Provider& font_provider, gfx::Texture& rt_texture, uint32_t font_size_pt);

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
