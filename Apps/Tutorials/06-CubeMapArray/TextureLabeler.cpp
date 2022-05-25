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

FILE: TextureLabeler.cpp
Renders text labels to the faces of cube-map texture array

******************************************************************************/

#include "TextureLabeler.h"

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/UserInterface/Context.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>

namespace Methane::Tutorials
{

static TextureLabeler::SliceDesc GetSliceDesc(Data::Size array_index, Data::Size depth_index,
                                              const TextureLabeler::CubeSliceDescs& cube_slice_descs,
                                              const gfx::Texture::Settings& rt_texture_settings)
{
    TextureLabeler::SliceDesc slice_desc = cube_slice_descs[depth_index % cube_slice_descs.size()];
    if (rt_texture_settings.dimension_type == gfx::Texture::DimensionType::Cube)
        return slice_desc;
    
    if (rt_texture_settings.dimension_type == gfx::Texture::DimensionType::CubeArray)
        slice_desc.label = fmt::format("{}{}", array_index, slice_desc.label);
    else
        slice_desc.label = fmt::format("{}{}", array_index, depth_index);

    return slice_desc;
}

TextureLabeler::TextureLabeler(gui::Context& gui_context, const Data::Provider& font_provider, gfx::Texture& rt_texture,
                               uint32_t font_size_pt, const gfx::Color4F& text_color,
                               const CubeSliceDescs& cube_slice_descs)
    : m_gui_context(gui_context)
    , m_rt_texture(rt_texture)
    , m_font(gui::Font::Library::Get().GetFont(font_provider, gui::Font::Settings{
        { "Face Labels",  "Fonts/RobotoMono/RobotoMono-Regular.ttf", font_size_pt }, 96U, U"XYZ+-:0123456789"
    }))
{
    const gfx::Texture::Settings& rt_texture_settings = m_rt_texture.GetSettings();
    const gfx::SubResource::Count& sub_res_count      = m_rt_texture.GetSubresourceCount();

    using namespace magic_enum::bitwise_operators;
    META_CHECK_ARG_TRUE(magic_enum::enum_contains(rt_texture_settings.usage_mask & gfx::Texture::Usage::RenderTarget));

    gfx::RenderPattern::Settings render_pattern_settings
    {
        {
            gfx::RenderPattern::ColorAttachment(
                0U, rt_texture_settings.pixel_format, 1U,
                gfx::RenderPattern::ColorAttachment::LoadAction::Clear,
                gfx::RenderPattern::ColorAttachment::StoreAction::Store)
        },
        std::nullopt, // No depth attachment
        std::nullopt, // No stencil attachment
        gfx::RenderPass::Access::ShaderResources |
        gfx::RenderPass::Access::Samplers,
        false // intermediate render pass
    };

    gui::Text::SettingsUtf32 slice_text_settings
    {
        {}, {},
        gui::UnitRect
        {
            gui::Units::Pixels,
            gfx::Point2I(),
            rt_texture_settings.dimensions.AsRectSize()
        },
        gui::Text::Layout
        {
            gui::Text::Wrap::None,
            gui::Text::HorizontalAlignment::Center,
            gui::Text::VerticalAlignment::Center,
        },
        text_color,
        false
    };

    const std::string& rt_texture_name = m_rt_texture.GetName();
    Refs<gfx::CommandList> slice_render_cmd_list_refs;
    for(Data::Size array_index = 0U; array_index < sub_res_count.GetArraySize(); ++array_index)
    {
        for(Data::Size depth_index = 0U; depth_index < sub_res_count.GetDepth(); ++depth_index)
        {
            m_slices.emplace_back(GetSliceDesc(array_index, depth_index, cube_slice_descs, rt_texture_settings));
            TextureLabeler::Slice& slice = m_slices.back();

            render_pattern_settings.color_attachments[0].clear_color = slice.color;
            slice.render_pattern_ptr = gfx::RenderPattern::Create(m_gui_context.GetRenderContext(), render_pattern_settings);
            slice.render_pass_ptr    = gfx::RenderPass::Create(*slice.render_pattern_ptr, {
                { gfx::Texture::Location(rt_texture, gfx::SubResource::Index(depth_index, array_index), {}, gfx::Texture::DimensionType::Tex2D) },
                rt_texture_settings.dimensions.AsRectSize()
            });

            slice_text_settings.name = slice.label + " Slice Label";
            slice_text_settings.text = gui::Font::ConvertUtf8To32(slice.label);
            slice.render_cmd_list_ptr = gfx::RenderCommandList::Create(m_gui_context.GetRenderCommandQueue(), *slice.render_pass_ptr);
            slice.render_cmd_list_ptr->SetName(fmt::format("Render Texture '{}' Slice {}:{} Label", rt_texture_name, array_index, depth_index));
            slice_render_cmd_list_refs.emplace_back(*slice.render_cmd_list_ptr);

            slice.label_text_ptr = std::make_shared<gui::Text>(m_gui_context, *slice.render_pattern_ptr, m_font, slice_text_settings);
            slice.label_text_ptr->Update(rt_texture_settings.dimensions.AsRectSize());
        }
    }

    m_slice_cmd_list_set_ptr = gfx::CommandListSet::Create(slice_render_cmd_list_refs);
}

void TextureLabeler::Render()
{
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group_ptr, "Texture Faces Rendering");
    for(const Slice& slice : m_slices)
    {
        META_CHECK_ARG_NOT_NULL(slice.label_text_ptr);
        META_CHECK_ARG_NOT_NULL(slice.render_cmd_list_ptr);
        slice.label_text_ptr->Draw(*slice.render_cmd_list_ptr, s_debug_group_ptr.get());
        slice.render_cmd_list_ptr->Commit();
    }

    m_gui_context.GetRenderCommandQueue().Execute(*m_slice_cmd_list_set_ptr);
}

} // namespace Methane::Tutorials
