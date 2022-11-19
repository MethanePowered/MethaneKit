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

#include <Methane/Tutorials/TextureLabeler.h>
#include <Methane/Graphics/RHI/IDevice.h>
#include <Methane/Graphics/RHI/IRenderCommandList.h>
#include <Methane/Graphics/RHI/ITexture.h>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
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
                                              const rhi::ITexture::Settings& rt_texture_settings,
                                              const rhi::SubResource::Count& sub_res_count)
{
    TextureLabeler::SliceDesc slice_desc = cube_slice_descs[depth_index % cube_slice_descs.size()];
    if (rt_texture_settings.dimension_type == rhi::TextureDimensionType::Cube)
        return slice_desc;

    if (rt_texture_settings.dimension_type == rhi::TextureDimensionType::CubeArray)
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

TextureLabeler::TextureLabeler(gui::Context& gui_context, const Data::IProvider& font_provider,
                               rhi::ITexture& rt_texture, rhi::ResourceState rt_texture_final_state, const Settings& settings)
    : m_gui_context(gui_context)
    , m_rt_texture(rt_texture)
    , m_font(gui::Font::Library::Get().GetFont(font_provider, gui::Font::Settings{
        { "Face Labels",  "Fonts/RobotoMono/RobotoMono-Regular.ttf", settings.font_size_pt }, 96U, U"XYZ+-:0123456789"
    }))
{
    const rhi::ITexture::Settings& rt_texture_settings = m_rt_texture.GetSettings();
    const rhi::SubResource::Count& sub_res_count       = m_rt_texture.GetSubresourceCount();

    using namespace magic_enum::bitwise_operators;
    META_CHECK_ARG_TRUE(static_cast<bool>(rt_texture_settings.usage_mask & rhi::ITexture::Usage::RenderTarget));

    m_texture_face_render_pattern_ptr = rhi::IRenderPattern::Create(m_gui_context.GetRenderContext(),
                                                                    rhi::IRenderPattern::Settings
        {
            rhi::IRenderPattern::ColorAttachments
            {
                rhi::IRenderPattern::ColorAttachment(
                    0U, rt_texture_settings.pixel_format, 1U,
                    rhi::IRenderPattern::ColorAttachment::LoadAction::Clear,
                    rhi::IRenderPattern::ColorAttachment::StoreAction::Store,
                    settings.border_color)
            },
            std::nullopt, // No depth attachment
            std::nullopt, // No stencil attachment
            rhi::IRenderPass::Access::ShaderResources |
            rhi::IRenderPass::Access::Samplers,
            false // intermediate render pass
        });

    const std::string& rt_texture_name = m_rt_texture.GetName();
    m_texture_face_render_pattern_ptr->SetName(fmt::format("Texture '{}' Face Render Pattern", rt_texture_name));

    gui::Text::SettingsUtf32 slice_text_settings
    {
        "",
        {},
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
        false,
    };
    slice_text_settings.SetStateName(fmt::format("Texture '{}' Face Label Text", rt_texture_name));

    Refs<rhi::ICommandList> slice_render_cmd_list_refs;
    for(Data::Size array_index = 0U; array_index < sub_res_count.GetArraySize(); ++array_index)
    {
        for(Data::Size depth_index = 0U; depth_index < sub_res_count.GetDepth(); ++depth_index)
        {
            m_slices.emplace_back(GetSliceDesc(array_index, depth_index, settings.cube_slice_descs, rt_texture_settings, sub_res_count));
            TextureLabeler::Slice& slice = m_slices.back();

            slice.render_pass_ptr = rhi::IRenderPass::Create(*m_texture_face_render_pattern_ptr, {
                { rhi::ITexture::View(rt_texture, rhi::SubResource::Index(depth_index, array_index), {}, rhi::TextureDimensionType::Tex2D) },
                rt_texture_settings.dimensions.AsRectSize()
            });
            slice.render_pass_ptr->SetName(fmt::format("Texture '{}' Slice {}:{} Render Pass", rt_texture_name, array_index, depth_index));

            slice.render_cmd_list_ptr = rhi::IRenderCommandList::Create(m_gui_context.GetRenderCommandQueue(), *slice.render_pass_ptr);
            slice.render_cmd_list_ptr->SetName(fmt::format("Render Texture '{}' Slice {}:{} Label", rt_texture_name, array_index, depth_index));
            slice_render_cmd_list_refs.emplace_back(*slice.render_cmd_list_ptr);

            slice_text_settings.name = fmt::format("Texture '{}' Slice {}:{} Label Text", rt_texture_name, array_index, depth_index);
            slice_text_settings.text = gui::Font::ConvertUtf8To32(slice.label);

            slice.label_text_ptr = std::make_shared<gui::Text>(m_gui_context, *m_texture_face_render_pattern_ptr, m_font, slice_text_settings);
            slice.label_text_ptr->Update(rt_texture_settings.dimensions.AsRectSize());

            slice.bg_quad_ptr = std::make_shared<gfx::ScreenQuad>(m_gui_context.GetRenderCommandQueue(), *m_texture_face_render_pattern_ptr,
                gfx::ScreenQuad::Settings
                {
                    fmt::format("Texture '{}' Slice BG Quad {}:{}", rt_texture_name, array_index, depth_index),
                    gfx::FrameRect(settings.border_width_px, settings.border_width_px,
                                   rt_texture_settings.dimensions.GetWidth()  - 2 * settings.border_width_px,
                                   rt_texture_settings.dimensions.GetHeight() - 2 * settings.border_width_px),
                    false,
                    slice.color,
                    gfx::ScreenQuad::TextureMode::Disabled
                });
        }
    }

    if (rt_texture_final_state != rhi::ResourceState::Undefined &&
        rhi::ISystem::GetNativeApi() != rhi::NativeApi::Metal) // No need in resource state transition barriers in Metal
    {
        m_ending_render_pattern_ptr = rhi::IRenderPattern::Create(m_gui_context.GetRenderContext(), {
            rhi::IRenderPattern::ColorAttachments{ }, std::nullopt, std::nullopt, rhi::IRenderPass::Access::ShaderResources, false
        });
        m_ending_render_pass_ptr = rhi::IRenderPass::Create(*m_ending_render_pattern_ptr, { { }, rt_texture_settings.dimensions.AsRectSize() });
        m_ending_render_cmd_list_ptr = rhi::IRenderCommandList::Create(m_gui_context.GetRenderCommandQueue(), *m_ending_render_pass_ptr);
        m_ending_render_cmd_list_ptr->SetName(fmt::format("Render Texture State Transition", rt_texture_name));
        m_ending_resource_barriers_ptr = rhi::IResourceBarriers::Create({
            { m_rt_texture, rhi::ResourceState::RenderTarget, rt_texture_final_state }
        });
        slice_render_cmd_list_refs.emplace_back(*m_ending_render_cmd_list_ptr);
    }

    m_render_cmd_list_set_ptr = rhi::ICommandListSet::Create(slice_render_cmd_list_refs);
}

void TextureLabeler::Render()
{
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group_ptr, "Texture Faces Rendering");
    for (const Slice& slice : m_slices)
    {
        META_CHECK_ARG_NOT_NULL(slice.bg_quad_ptr);
        META_CHECK_ARG_NOT_NULL(slice.label_text_ptr);
        META_CHECK_ARG_NOT_NULL(slice.render_cmd_list_ptr);

        slice.bg_quad_ptr->Draw(*slice.render_cmd_list_ptr, s_debug_group_ptr.get());
        slice.label_text_ptr->Draw(*slice.render_cmd_list_ptr, s_debug_group_ptr.get());
        slice.render_cmd_list_ptr->Commit();
    }

    if (m_ending_resource_barriers_ptr)
    {
        m_ending_render_cmd_list_ptr->Reset();
        m_ending_render_cmd_list_ptr->SetResourceBarriers(*m_ending_resource_barriers_ptr);
        m_ending_render_cmd_list_ptr->Commit();
    }

    m_gui_context.GetRenderCommandQueue().Execute(*m_render_cmd_list_set_ptr);
}

} // namespace Methane::Tutorials
