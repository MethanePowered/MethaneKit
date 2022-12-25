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
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/CommandListSet.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/ScreenQuad.h>
#include <Methane/UserInterface/Context.h>
#include <Methane/UserInterface/Font.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/Checks.hpp>

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
                               const rhi::Texture& rt_texture, rhi::ResourceState rt_texture_final_state,
                               const Settings& settings)
    : m_gui_context(gui_context)
    , m_rt_texture(rt_texture)
    , m_font(gui::Font::Library::Get().GetFont(font_provider, gui::Font::Settings{
        { "Face Labels",  "Fonts/RobotoMono/RobotoMono-Regular.ttf", settings.font_size_pt }, 96U, U"XYZ+-:0123456789"
    }))
{
    const rhi::ITexture::Settings& rt_texture_settings = m_rt_texture.GetSettings();
    const rhi::SubResource::Count& sub_res_count       = m_rt_texture.GetSubresourceCount();

    META_CHECK_ARG_TRUE(rt_texture_settings.usage_mask.HasAnyBit(rhi::ResourceUsage::RenderTarget));

    m_texture_face_render_pattern.Init(m_gui_context.GetRenderContext(),
        rhi::RenderPattern::Settings
        {
            rhi::RenderPattern::ColorAttachments
            {
                rhi::RenderPattern::ColorAttachment(
                    0U, rt_texture_settings.pixel_format, 1U,
                    rhi::RenderPattern::ColorAttachment::LoadAction::Clear,
                    rhi::RenderPattern::ColorAttachment::StoreAction::Store,
                    settings.border_color)
            },
            std::nullopt, // No depth attachment
            std::nullopt, // No stencil attachment
            rhi::RenderPassAccessMask({ rhi::RenderPassAccess::ShaderResources, rhi::RenderPassAccess::Samplers }),
            false // intermediate render pass
        });

    const std::string_view rt_texture_name = m_rt_texture.GetName();
    m_texture_face_render_pattern.SetName(fmt::format("Texture '{}' Face Render Pattern", rt_texture_name));

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

            slice.render_pass.Init(m_texture_face_render_pattern, {
                { rhi::TextureView(rt_texture.GetInterface(), rhi::SubResource::Index(depth_index, array_index), {}, rhi::TextureDimensionType::Tex2D) },
                rt_texture_settings.dimensions.AsRectSize()
            });
            slice.render_pass.SetName(fmt::format("Texture '{}' Slice {}:{} Render Pass", rt_texture_name, array_index, depth_index));

            slice.render_cmd_list.Init(m_gui_context.GetRenderCommandQueue(), slice.render_pass);
            slice.render_cmd_list.SetName(fmt::format("Render Texture '{}' Slice {}:{} Label", rt_texture_name, array_index, depth_index));
            slice_render_cmd_list_refs.emplace_back(slice.render_cmd_list.GetInterface());

            slice_text_settings.name = fmt::format("Texture '{}' Slice {}:{} Label Text", rt_texture_name, array_index, depth_index);
            slice_text_settings.text = gui::Font::ConvertUtf8To32(slice.label);

            slice.label_text_ptr = std::make_shared<gui::Text>(m_gui_context, m_texture_face_render_pattern, m_font, slice_text_settings);
            slice.label_text_ptr->Update(rt_texture_settings.dimensions.AsRectSize());

            slice.bg_quad_ptr = std::make_shared<gfx::ScreenQuad>(m_gui_context.GetRenderCommandQueue(), m_texture_face_render_pattern,
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
        rhi::System::GetNativeApi() != rhi::NativeApi::Metal) // No need in resource state transition barriers in Metal
    {
        m_ending_render_pattern.Init(m_gui_context.GetRenderContext(), {
            rhi::IRenderPattern::ColorAttachments{ },
            std::nullopt, std::nullopt,
            rhi::RenderPassAccessMask(rhi::RenderPassAccess::ShaderResources),
            false
        });
        m_ending_render_pass.Init(m_ending_render_pattern, { { }, rt_texture_settings.dimensions.AsRectSize() });
        m_ending_render_cmd_list.Init(m_gui_context.GetRenderCommandQueue(), m_ending_render_pass);
        m_ending_render_cmd_list.SetName(fmt::format("Render Texture State Transition", rt_texture_name));
        m_ending_resource_barriers.Init({
            { m_rt_texture.GetInterface(), rhi::ResourceState::RenderTarget, rt_texture_final_state }
        });
        slice_render_cmd_list_refs.emplace_back(m_ending_render_cmd_list.GetInterface());
    }

    m_render_cmd_list_set.Init(slice_render_cmd_list_refs);
}

void TextureLabeler::Render()
{
    META_DEBUG_GROUP_VAR(s_debug_group, "Texture Faces Rendering");
    for (const Slice& slice : m_slices)
    {
        META_CHECK_ARG_NOT_NULL(slice.bg_quad_ptr);
        META_CHECK_ARG_NOT_NULL(slice.label_text_ptr);
        META_CHECK_ARG_TRUE(slice.render_cmd_list.IsInitialized());

        slice.bg_quad_ptr->Draw(slice.render_cmd_list, &s_debug_group);
        slice.label_text_ptr->Draw(slice.render_cmd_list, &s_debug_group);
        slice.render_cmd_list.Commit();
    }

    if (m_ending_resource_barriers.IsInitialized())
    {
        m_ending_render_cmd_list.Reset();
        m_ending_render_cmd_list.SetResourceBarriers(m_ending_resource_barriers);
        m_ending_render_cmd_list.Commit();
    }

    m_gui_context.GetRenderCommandQueue().Execute(m_render_cmd_list_set);
}

} // namespace Methane::Tutorials
