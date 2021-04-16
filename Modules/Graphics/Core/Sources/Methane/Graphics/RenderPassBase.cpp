/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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
#include "TextureBase.h"
#include "RenderCommandListBase.h"

#include <Methane/Data/BitMaskHelpers.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>

template<>
struct fmt::formatter<Methane::Graphics::RenderPass::ColorAttachment>
{
    template<typename FormatContext>
    [[nodiscard]] auto format(const Methane::Graphics::RenderPass::ColorAttachment& ca, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string>(ca)); }
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }
};

namespace Methane::Graphics
{

bool RenderPass::Attachment::operator==(const RenderPass::Attachment& other) const
{
    META_FUNCTION_TASK();
    return std::tie(texture_location, load_action, store_action) ==
           std::tie(other.texture_location, other.load_action, other.store_action);
}

bool RenderPass::Attachment::operator!=(const RenderPass::Attachment& other) const
{
    META_FUNCTION_TASK();
    return std::tie(texture_location, load_action, store_action) !=
           std::tie(other.texture_location, other.load_action, other.store_action);
}

RenderPass::Attachment::operator std::string() const
{
    META_FUNCTION_TASK();
    return fmt::format("attachment for {}, load={}, store={}",
                       static_cast<std::string>(texture_location),
                       magic_enum::enum_name(load_action),
                       magic_enum::enum_name(store_action));
}

bool RenderPass::ColorAttachment::operator==(const RenderPass::ColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) && clear_color == other.clear_color;
}

bool RenderPass::ColorAttachment::operator!=(const RenderPass::ColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator!=(other) || clear_color != other.clear_color;
}

RenderPass::ColorAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!texture_location.IsInitialized())
        return "  - No color attachment";

    return fmt::format("  - Color {}, clear_color={}",
                       Attachment::operator std::string(),
                       static_cast<std::string>(clear_color));
}

bool RenderPass::DepthAttachment::operator==(const RenderPass::DepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) && clear_value == other.clear_value;
}

bool RenderPass::DepthAttachment::operator!=(const RenderPass::DepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator!=(other) || clear_value != other.clear_value;
}

RenderPass::DepthAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!texture_location.IsInitialized())
        return "  - No depth attachment";

    return fmt::format("  - Depth {}, clear_value={}",
                       Attachment::operator std::string(),
                       clear_value);
}

bool RenderPass::StencilAttachment::operator==(const RenderPass::StencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) && clear_value == other.clear_value;
}

bool RenderPass::StencilAttachment::operator!=(const RenderPass::StencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator!=(other) || clear_value != other.clear_value;
}

RenderPass::StencilAttachment::operator std::string() const
{
    META_FUNCTION_TASK();
    if (!texture_location.IsInitialized())
        return "  - No stencil attachment";

    return fmt::format("  - Stencil {}, clear_value={}",
                       Attachment::operator std::string(),
                       clear_value);
}

bool RenderPass::Settings::operator==(const Settings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access_mask, is_final_pass) ==
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access_mask, other.is_final_pass);
}

bool RenderPass::Settings::operator!=(const Settings& other) const
{
    META_FUNCTION_TASK();
    return std::tie(color_attachments, depth_attachment, stencil_attachment, shader_access_mask, is_final_pass) !=
           std::tie(other.color_attachments, other.depth_attachment, other.stencil_attachment, other.shader_access_mask, other.is_final_pass);
}

RenderPass::Settings::operator std::string() const
{
    META_FUNCTION_TASK();
    std::string color_attachments_str = "  - No color attachments";
    if (!color_attachments.empty())
        color_attachments_str = fmt::format("{}", fmt::join(color_attachments, ";\n"));

    return fmt::format("{};\n{};\n{};\n  - shader_access_mask={}, {} pass.",
                       color_attachments_str,
                       static_cast<std::string>(depth_attachment),
                       static_cast<std::string>(stencil_attachment),
                       Data::GetBitMaskFlagNames(shader_access_mask),
                       (is_final_pass ? "final" : "intermediate"));
}

RenderPass::Attachment::Attachment(Texture::Location&& texture_location, LoadAction load_action, StoreAction store_action)
    : texture_location(std::move(texture_location))
    , load_action(load_action)
    , store_action(store_action)
{
    META_FUNCTION_TASK();
}

RenderPass::ColorAttachment::ColorAttachment(Texture::Location&& texture_location, LoadAction load_action, StoreAction store_action, const Color4F& clear_color)
    : Attachment(std::move(texture_location), load_action, store_action)
    , clear_color(clear_color)
{
    META_FUNCTION_TASK();
}

RenderPass::DepthAttachment::DepthAttachment(Texture::Location&& texture_location, LoadAction load_action, StoreAction store_action, Depth clear_value)
    : Attachment(std::move(texture_location), load_action, store_action)
    , clear_value(clear_value)
{
    META_FUNCTION_TASK();
}

RenderPass::StencilAttachment::StencilAttachment(Texture::Location&& texture_location, LoadAction load_action, StoreAction store_action, Stencil clear_value)
    : Attachment(std::move(texture_location), load_action, store_action)
    , clear_value(clear_value)
{
    META_FUNCTION_TASK();
}

RenderPassBase::RenderPassBase(RenderContextBase& context, const Settings& settings)
    : m_render_context(context)
    , m_settings(settings)
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
    m_begin_transition_barriers_ptr.reset();
    m_end_transition_barriers_ptr.reset();

    InitAttachmentStates();
    return true;
}

void RenderPassBase::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();
    m_non_frame_buffer_attachment_textures.clear();
    m_settings.depth_attachment.texture_location = Texture::Location();
    m_settings.stencil_attachment.texture_location = Texture::Location();
    for(ColorAttachment& color_attachment : m_settings.color_attachments)
    {
        color_attachment.texture_location = Texture::Location();
    }
}

void RenderPassBase::Begin(RenderCommandListBase& render_command_list)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE_DESCR(m_is_begun, "can not begin pass which was begun already and was not ended");

    SetAttachmentStates(Resource::State::RenderTarget, Resource::State::DepthWrite, m_begin_transition_barriers_ptr, render_command_list);
    m_is_begun = true;
}

void RenderPassBase::End(RenderCommandListBase& render_command_list)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(m_is_begun, "can not end render pass, which was not begun");

    if (m_settings.is_final_pass)
    {
        SetAttachmentStates(Resource::State::Present, { }, m_end_transition_barriers_ptr, render_command_list);
    }
    m_is_begun = false;
}

void RenderPassBase::InitAttachmentStates() const
{
    META_FUNCTION_TASK();
    Ptr<Resource::Barriers> transition_barriers_ptr;
    for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
    {
        if (color_texture_ref.get().GetState() == Resource::State::Common)
            color_texture_ref.get().SetState(Resource::State::Present, transition_barriers_ptr);
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

const Refs<TextureBase>& RenderPassBase::GetColorAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_color_attachment_textures.empty())
        return m_color_attachment_textures;

    m_color_attachment_textures.reserve(m_settings.color_attachments.size());
    for (const ColorAttachment& color_attach : m_settings.color_attachments)
    {
        META_CHECK_ARG_TRUE_DESCR(color_attach.texture_location.IsInitialized(), "can not use color attachment without texture");
        m_color_attachment_textures.push_back(static_cast<TextureBase&>(color_attach.texture_location.GetTexture()));
    }
    return m_color_attachment_textures;
}

TextureBase* RenderPassBase::GetDepthAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (!m_p_depth_attachment_texture && m_settings.depth_attachment.texture_location.IsInitialized())
    {
        m_p_depth_attachment_texture = static_cast<TextureBase*>(m_settings.depth_attachment.texture_location.GetTexturePtr().get());
    }
    return m_p_depth_attachment_texture;
}

const Ptrs<TextureBase>& RenderPassBase::GetNonFrameBufferAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_non_frame_buffer_attachment_textures.empty())
        return m_non_frame_buffer_attachment_textures;

    m_non_frame_buffer_attachment_textures.reserve(m_settings.color_attachments.size() + 2);

    for (const ColorAttachment& color_attach : m_settings.color_attachments)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(color_attach.texture_location.IsInitialized(), "can not use color attachment without texture");

        Ptr<TextureBase> color_attachment_ptr = std::static_pointer_cast<TextureBase>(color_attach.texture_location.GetTexturePtr());
        if (color_attachment_ptr->GetSettings().type == Texture::Type::FrameBuffer)
            continue;

        m_non_frame_buffer_attachment_textures.emplace_back(std::move(color_attachment_ptr));
    }

    if (m_settings.depth_attachment.texture_location.IsInitialized())
    {
        m_non_frame_buffer_attachment_textures.emplace_back(std::static_pointer_cast<TextureBase>(m_settings.depth_attachment.texture_location.GetTexturePtr()));
    }

    if (m_settings.stencil_attachment.texture_location.IsInitialized())
    {
        m_non_frame_buffer_attachment_textures.emplace_back(std::static_pointer_cast<TextureBase>(m_settings.stencil_attachment.texture_location.GetTexturePtr()));
    }

    return m_non_frame_buffer_attachment_textures;
}

} // namespace Methane::Graphics
