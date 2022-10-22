/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RenderPassBase.cpp
Base implementation of the render pass interface.

******************************************************************************/

#include "RenderPassBase.h"
#include "RenderContextBase.h"
#include "TextureBase.h"
#include "RenderCommandListBase.h"

#include <Methane/Data/BitMaskHelpers.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::RenderPattern::ColorAttachment>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::RenderPattern::ColorAttachment& ca, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(ca)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics
{

RenderPattern::Attachment::Attachment(Data::Index attachment_index, PixelFormat format,
                                      Data::Size samples_count, LoadAction load_action, StoreAction store_action)
    : attachment_index(attachment_index)
    , format(format)
    , samples_count(samples_count)
    , load_action(load_action)
    , store_action(store_action)
{
    META_FUNCTION_TASK();
}

bool RenderPattern::Attachment::operator==(const RenderPattern::Attachment& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachment_index, format, samples_count, load_action, store_action) ==
           std::tie(other.attachment_index, format, samples_count, other.load_action, other.store_action);
}

bool RenderPattern::Attachment::operator!=(const RenderPattern::Attachment& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachment_index, format, samples_count, load_action, store_action) !=
           std::tie(other.attachment_index, format, samples_count, other.load_action, other.store_action);
}

RenderPattern::Attachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("attachment id {}: format={}, samples={}, load={}, store={}",
                       attachment_index,
                       magic_enum::enum_name(format),
                       samples_count,
                       magic_enum::enum_name(load_action),
                       magic_enum::enum_name(store_action));
}

RenderPattern::ColorAttachment::ColorAttachment(Data::Index attachment_index, PixelFormat format, Data::Size samples_count,
                                                LoadAction load_action, StoreAction store_action, const Color4F& clear_color)
    : Attachment(attachment_index, format, samples_count, load_action, store_action)
    , clear_color(clear_color)
{
    META_FUNCTION_TASK();
}

bool RenderPattern::ColorAttachment::operator==(const RenderPattern::ColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) && clear_color == other.clear_color;
}

bool RenderPattern::ColorAttachment::operator!=(const RenderPattern::ColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator!=(other) || clear_color != other.clear_color;
}

RenderPattern::ColorAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Color {}, clear_color={}",
                       Attachment::operator std::string(),
                       static_cast<std::string>(clear_color));
}

RenderPattern::DepthAttachment::DepthAttachment(Data::Index attachment_index, PixelFormat format, Data::Size samples_count,
                                                LoadAction load_action, StoreAction store_action, Depth clear_value)
    : Attachment(attachment_index, format, samples_count, load_action, store_action)
    , clear_value(clear_value)
{
    META_FUNCTION_TASK();
}

bool RenderPattern::DepthAttachment::operator==(const RenderPattern::DepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) && clear_value == other.clear_value;
}

bool RenderPattern::DepthAttachment::operator!=(const RenderPattern::DepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator!=(other) || clear_value != other.clear_value;
}

RenderPattern::DepthAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Depth {}, clear_value={}",
                       Attachment::operator std::string(),
                       clear_value);
}

RenderPattern::StencilAttachment::StencilAttachment(Data::Index attachment_index, PixelFormat format, Data::Size samples_count,
                                                    LoadAction load_action, StoreAction store_action, Stencil clear_value)
    : Attachment(attachment_index, format, samples_count, load_action, store_action)
    , clear_value(clear_value)
{
    META_FUNCTION_TASK();
}

bool RenderPattern::StencilAttachment::operator==(const RenderPattern::StencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) && clear_value == other.clear_value;
}

bool RenderPattern::StencilAttachment::operator!=(const RenderPattern::StencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator!=(other) || clear_value != other.clear_value;
}

RenderPattern::StencilAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("  - Stencil {}, clear_value={}",
                       Attachment::operator std::string(),
                       clear_value);
}

RenderPatternBase::RenderPatternBase(RenderContextBase& render_context, const Settings& settings)
    : m_render_context_ptr(std::dynamic_pointer_cast<RenderContextBase>(render_context.GetPtr()))
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

const IRenderContext& RenderPatternBase::GetRenderContext() const noexcept
{
    META_FUNCTION_TASK();
    return *m_render_context_ptr;
}

IRenderContext& RenderPatternBase::GetRenderContext() noexcept
{
    META_FUNCTION_TASK();
    return *m_render_context_ptr;
}

Data::Size RenderPatternBase::GetAttachmentCount() const noexcept
{
    META_FUNCTION_TASK();
    auto attachment_count = static_cast<Data::Size>(m_settings.color_attachments.size());
    if (m_settings.depth_attachment)
        attachment_count++;
    if (m_settings.stencil_attachment)
        attachment_count++;
    return attachment_count;
}

AttachmentFormats RenderPatternBase::GetAttachmentFormats() const noexcept
{
    META_FUNCTION_TASK();
    AttachmentFormats attachment_formats;

    attachment_formats.colors.reserve(m_settings.color_attachments.size());
    std::transform(m_settings.color_attachments.begin(), m_settings.color_attachments.end(), std::back_inserter(attachment_formats.colors),
                   [](const ColorAttachment& color_attachment) { return color_attachment.format; });

    if (m_settings.depth_attachment)
        attachment_formats.depth = m_settings.depth_attachment->format;

    if (m_settings.stencil_attachment)
        attachment_formats.stencil = m_settings.stencil_attachment->format;

    return attachment_formats;
}

bool RenderPattern::Settings::operator==(const Settings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access_mask, is_final_pass) ==
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access_mask, other.is_final_pass);
}

bool RenderPattern::Settings::operator!=(const Settings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access_mask, is_final_pass) !=
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access_mask, other.is_final_pass);
}

RenderPattern::Settings::operator std::string() const
{
    META_FUNCTION_TASK();
    std::string color_attachments_str = "  - No color attachments";
    if (!color_attachments.empty())
        color_attachments_str = fmt::format("{}", fmt::join(color_attachments, ";\n"));

    return fmt::format("{};\n{};\n{};\n  - shader_access_mask={}, {} pass.",
                       color_attachments_str,
                       depth_attachment   ? static_cast<std::string>(*depth_attachment)   : "  - No stencil attachment",
                       stencil_attachment ? static_cast<std::string>(*stencil_attachment) : "  - No depth attachment",
                       Data::GetBitMaskFlagNames(shader_access_mask),
                       (is_final_pass ? "final" : "intermediate"));
}

bool RenderPass::Settings::operator==(const Settings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachments, frame_size) ==
           std::tie(other.attachments, other.frame_size);
}

bool RenderPass::Settings::operator!=(const Settings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(attachments, frame_size) !=
           std::tie(other.attachments, other.frame_size);
}

RenderPassBase::RenderPassBase(RenderPatternBase& render_pattern, const Settings& settings, bool update_attachment_states)
    : m_pattern_base_ptr(std::dynamic_pointer_cast<RenderPatternBase>(render_pattern.GetBasePtr()))
    , m_settings(settings)
    , m_update_attachment_states(update_attachment_states)
{
    META_FUNCTION_TASK();
    InitAttachmentStates();
}

bool RenderPassBase::Update(const RenderPass::Settings& settings)
{
    META_FUNCTION_TASK();
    if (m_settings == settings)
        return false;

    m_settings = settings;

    m_non_frame_buffer_attachment_textures.clear();
    m_color_attachment_textures.clear();
    m_p_depth_attachment_texture = nullptr;

    InitAttachmentStates();
    return true;
}

void RenderPassBase::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();
    m_non_frame_buffer_attachment_textures.clear();
    m_settings.attachments.clear();
}

void RenderPassBase::Begin(RenderCommandListBase&)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE_DESCR(m_is_begun, "can not begin pass which was begun already and was not ended");

    if (m_update_attachment_states)
    {
        SetAttachmentStates(Resource::State::RenderTarget, Resource::State::DepthWrite);
    }
    m_is_begun = true;
}

void RenderPassBase::End(RenderCommandListBase&)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(m_is_begun, "can not end render pass, which was not begun");

    if (m_update_attachment_states && GetPatternBase().GetSettings().is_final_pass)
    {
        SetAttachmentStates(Resource::State::Present, { });
    }
    m_is_begun = false;
}

void RenderPassBase::InitAttachmentStates() const
{
    META_FUNCTION_TASK();
    const bool is_final_pass = GetPatternBase().GetSettings().is_final_pass;
    const Resource::State color_attachment_state = is_final_pass ? Resource::State::Present : Resource::State::RenderTarget;
    for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
    {
        if (color_texture_ref.get().GetState() == Resource::State::Common ||
            color_texture_ref.get().GetState() == Resource::State::Undefined)
            color_texture_ref.get().SetState(color_attachment_state);
    }
}

void RenderPassBase::SetAttachmentStates(const std::optional<Resource::State>& color_state,
                                         const std::optional<Resource::State>& depth_state) const
{
    META_FUNCTION_TASK();
    if (color_state)
    {
        for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
        {
            color_texture_ref.get().SetState(*color_state);
        }
    }

    if (depth_state)
    {
        if (TextureBase* p_depth_texture = GetDepthAttachmentTexture();
            p_depth_texture)
        {
            p_depth_texture->SetState(*depth_state);
        }
    }
}

void RenderPassBase::SetAttachmentStates(const std::optional<Resource::State>& color_state,
                                         const std::optional<Resource::State>& depth_state,
                                         Ptr<Resource::Barriers>& transition_barriers_ptr,
                                         RenderCommandListBase& render_command_list) const
{
    META_FUNCTION_TASK();
    bool attachment_states_changed = false;

    if (color_state)
    {
        for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
        {
            attachment_states_changed |= color_texture_ref.get().SetState(*color_state, transition_barriers_ptr);
        }
    }

    if (depth_state)
    {
        if (TextureBase* p_depth_texture = GetDepthAttachmentTexture();
            p_depth_texture)
        {
            attachment_states_changed |= p_depth_texture->SetState(*depth_state, transition_barriers_ptr);
        }
    }

    if (transition_barriers_ptr && attachment_states_changed)
    {
        render_command_list.SetResourceBarriers(*transition_barriers_ptr);
    }
}

const Texture::View& RenderPassBase::GetAttachmentTextureView(const Attachment& attachment) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(attachment.attachment_index, m_settings.attachments.size(),
                              "attachment index is out of bounds of render pass attachments array");
    return m_settings.attachments[attachment.attachment_index];
}

const Refs<TextureBase>& RenderPassBase::GetColorAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_color_attachment_textures.empty())
        return m_color_attachment_textures;

    const ColorAttachments& color_attachments = GetPatternBase().GetSettings().color_attachments;
    m_color_attachment_textures.reserve(color_attachments.size());
    for (const ColorAttachment& color_attach : color_attachments)
    {
        m_color_attachment_textures.push_back(static_cast<TextureBase&>(GetAttachmentTextureView(color_attach).GetTexture()));
    }
    return m_color_attachment_textures;
}

TextureBase* RenderPassBase::GetDepthAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (m_p_depth_attachment_texture)
        return m_p_depth_attachment_texture;

    const Opt<DepthAttachment>& depth_attachment_opt = GetPatternBase().GetSettings().depth_attachment;
    if (!depth_attachment_opt)
        return nullptr;

    m_p_depth_attachment_texture = static_cast<TextureBase*>(GetAttachmentTextureView(*depth_attachment_opt).GetTexturePtr().get());
    return m_p_depth_attachment_texture;
}

TextureBase* RenderPassBase::GetStencilAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (m_p_stencil_attachment_texture)
        return m_p_stencil_attachment_texture;

    const Opt<StencilAttachment>& stencil_attachment_opt = GetPatternBase().GetSettings().stencil_attachment;
    if (!stencil_attachment_opt)
        return nullptr;

    m_p_stencil_attachment_texture = static_cast<TextureBase*>(GetAttachmentTextureView(*stencil_attachment_opt).GetTexturePtr().get());
    return m_p_stencil_attachment_texture;
}

const Ptrs<TextureBase>& RenderPassBase::GetNonFrameBufferAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_non_frame_buffer_attachment_textures.empty())
        return m_non_frame_buffer_attachment_textures;

    m_non_frame_buffer_attachment_textures.reserve(m_settings.attachments.size());

    for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
    {
        Ptr<TextureBase> color_attachment_ptr = color_texture_ref.get().GetPtr<TextureBase>();
        if (color_attachment_ptr->GetSettings().type == Texture::Type::FrameBuffer)
            continue;

        m_non_frame_buffer_attachment_textures.emplace_back(std::move(color_attachment_ptr));
    }

    if (TextureBase* depth_texture_ptr = GetDepthAttachmentTexture();
        depth_texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(depth_texture_ptr->GetPtr<TextureBase>());
    }

    if (TextureBase* stencil_texture_ptr = GetStencilAttachmentTexture();
        stencil_texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(stencil_texture_ptr->GetPtr<TextureBase>());
    }

    return m_non_frame_buffer_attachment_textures;
}

} // namespace Methane::Graphics
