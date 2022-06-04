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

#include <Methane/Samples/TextureLabeler.h>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/Texture.h>
#include <Methane/Graphics/RenderPass.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Graphics/ScreenQuad.h>
#include <Methane/UserInterface/Context.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>

namespace Methane::Tutorials
{

static TextureLabeler::SliceDesc GetSliceDesc(Data::Size array_index, Data::Size depth_index,
                                              const TextureLabeler::CubeSliceDescs& cube_slice_descs,
                                              const gfx::Texture::Settings& rt_texture_settings,
                                              const gfx::SubResource::Count& sub_res_count)
{
    TextureLabeler::SliceDesc slice_desc = cube_slice_descs[depth_index % cube_slice_descs.size()];
    if (rt_texture_settings.dimension_type == gfx::Texture::DimensionType::Cube)
        return slice_desc;

    if (rt_texture_settings.dimension_type == gfx::Texture::DimensionType::CubeArray)
    {
        slice_desc.label = fmt::format("{}{}", array_index, slice_desc.label);
        return slice_desc;
    }

    if (sub_res_count.GetArraySize() > 1 && sub_res_count.GetDepth() > 1)
    {
        slice_desc.label = fmt::format("{}:{}", array_index, depth_index);
        return slice_desc;
    }

    if (sub_res_count.GetArraySize() > 1)
    {
        const Data::Index desc_index = array_index % cube_slice_descs.size();
        slice_desc = cube_slice_descs[desc_index];
        slice_desc.label = fmt::format("{}", array_index);
    }
    else
    {
        slice_desc.label = fmt::format("{}", depth_index);
    }
    return slice_desc;
}

TextureLabeler::TextureLabeler(gui::Context& gui_context, const Data::Provider& font_provider,
                               gfx::Texture& rt_texture, gfx::ResourceState rt_texture_final_state, const Settings& settings)
    : m_gui_context(gui_context)
    , m_rt_texture(rt_texture)
    , m_font(gui::Font::Library::Get().GetFont(font_provider, gui::Font::Settings{
        { "Face Labels",  "Fonts/RobotoMono/RobotoMono-Regular.ttf", settings.font_size_pt }, 96U, U"XYZ+-:0123456789"
    }))
{
    const gfx::Texture::Settings& rt_texture_settings = m_rt_texture.GetSettings();
    const gfx::SubResource::Count& sub_res_count      = m_rt_texture.GetSubresourceCount();

    using namespace magic_enum::bitwise_operators;
    META_CHECK_ARG_TRUE(magic_enum::enum_contains(rt_texture_settings.usage_mask & gfx::Texture::Usage::RenderTarget));

    gfx::RenderPattern::Settings render_pattern_settings
    {
        gfx::RenderPattern::ColorAttachments
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
        settings.text_color,
        false
    };

    const std::string& rt_texture_name = m_rt_texture.GetName();
    Refs<gfx::CommandList> slice_render_cmd_list_refs;
    for(Data::Size array_index = 0U; array_index < sub_res_count.GetArraySize(); ++array_index)
    {
        for(Data::Size depth_index = 0U; depth_index < sub_res_count.GetDepth(); ++depth_index)
        {
            m_slices.emplace_back(GetSliceDesc(array_index, depth_index, settings.cube_slice_descs, rt_texture_settings, sub_res_count));
            TextureLabeler::Slice& slice = m_slices.back();

            render_pattern_settings.color_attachments[0].clear_color = settings.border_width_px ? settings.border_color : slice.color;
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

            if (settings.border_width_px)
            {
                slice.screen_quad_ptr = std::make_shared<gfx::ScreenQuad>(m_gui_context.GetRenderCommandQueue(), *slice.render_pattern_ptr,
                    gfx::ScreenQuad::Settings
                    {
                        fmt::format("Texture '{}' Slice Quad {}:{}", rt_texture_name, array_index, depth_index),
                        gfx::FrameRect(settings.border_width_px, settings.border_width_px,
                                       rt_texture_settings.dimensions.GetWidth()  - 2 * settings.border_width_px,
                                       rt_texture_settings.dimensions.GetHeight() - 2 * settings.border_width_px),
                        false,
                        slice.color,
                        gfx::ScreenQuad::TextureMode::Disabled
                    });
            }
        }
    }

    if (rt_texture_final_state != gfx::Resource::State::Undefined)
        m_ending_resource_barriers_ptr = gfx::Resource::Barriers::Create({
            { m_rt_texture, gfx::Resource::State::RenderTarget, rt_texture_final_state }
        });

    m_slice_cmd_list_set_ptr = gfx::CommandListSet::Create(slice_render_cmd_list_refs);
}

void TextureLabeler::Render()
{
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group_ptr, "Texture Faces Rendering");
    for(size_t slice_index = 0U; slice_index < m_slices.size(); ++slice_index)
    {
        const Slice& slice = m_slices[slice_index];
        META_CHECK_ARG_NOT_NULL(slice.label_text_ptr);
        META_CHECK_ARG_NOT_NULL(slice.render_cmd_list_ptr);

        if (slice.screen_quad_ptr)
            slice.screen_quad_ptr->Draw(*slice.render_cmd_list_ptr, s_debug_group_ptr.get());

        slice.label_text_ptr->Draw(*slice.render_cmd_list_ptr, s_debug_group_ptr.get());

        if (m_ending_resource_barriers_ptr && slice_index == m_slices.size() - 1)
            slice.render_cmd_list_ptr->SetResourceBarriers(*m_ending_resource_barriers_ptr);

        slice.render_cmd_list_ptr->Commit();
    }

    m_gui_context.GetRenderCommandQueue().Execute(*m_slice_cmd_list_set_ptr);
}

} // namespace Methane::Tutorials