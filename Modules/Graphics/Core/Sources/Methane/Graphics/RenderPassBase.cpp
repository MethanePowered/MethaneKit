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

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

bool RenderPass::Attachment::operator==(const RenderPass::Attachment& other) const
{
    META_FUNCTION_TASK();
    return texture_ptr  == other.texture_ptr &&
           level        == other.level &&
           slice        == other.slice &&
           depth_plane  == other.depth_plane &&
           load_action  == other.load_action &&
           store_action == other.store_action;
}

bool RenderPass::ColorAttachment::operator==(const RenderPass::ColorAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) &&
           clear_color == other.clear_color;
}

bool RenderPass::DepthAttachment::operator==(const RenderPass::DepthAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) &&
           clear_value == other.clear_value;
}

bool RenderPass::StencilAttachment::operator==(const RenderPass::StencilAttachment& other) const
{
    META_FUNCTION_TASK();
    return Attachment::operator==(other) &&
           clear_value == other.clear_value;
}

bool RenderPass::Settings::operator==(const Settings& other) const
{
    META_FUNCTION_TASK();
    return color_attachments  == other.color_attachments &&
           depth_attachment   == other.depth_attachment &&
           stencil_attachment == other.stencil_attachment &&
           shader_access_mask == other.shader_access_mask &&
           is_final_pass      == other.is_final_pass;
}

bool RenderPass::Settings::operator!=(const Settings& other) const
{
    META_FUNCTION_TASK();
    return !operator==(other);
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
    m_settings.depth_attachment.texture_ptr.reset();
    m_settings.stencil_attachment.texture_ptr.reset();
    for(ColorAttachment& color_attachment : m_settings.color_attachments)
    {
        color_attachment.texture_ptr.reset();
    }
}

void RenderPassBase::Begin(RenderCommandListBase& render_command_list)
{
    META_FUNCTION_TASK();
    if (m_is_begun)
    {
        throw std::logic_error("Can not begin pass which was begun already and was not ended.");
    }

    SetAttachmentStates(ResourceBase::State::RenderTarget, ResourceBase::State::DepthWrite,
                        m_begin_transition_barriers_ptr, render_command_list);

    m_is_begun = true;
}

void RenderPassBase::End(RenderCommandListBase& render_command_list)
{
    META_FUNCTION_TASK();
    if (!m_is_begun)
    {
        throw std::logic_error("Can not end render pass, which was not begun.");
    }

    if (m_settings.is_final_pass)
    {
        SetAttachmentStates(ResourceBase::State::Present, { },
                            m_end_transition_barriers_ptr, render_command_list);
    }

    m_is_begun = false;
}

void RenderPassBase::InitAttachmentStates()
{
    Ptr<ResourceBase::Barriers> transition_barriers_ptr;
    for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
    {
        if (color_texture_ref.get().GetState() == ResourceBase::State::Common)
            color_texture_ref.get().SetState(ResourceBase::State::Present, transition_barriers_ptr);
    }
}

void RenderPassBase::SetAttachmentStates(const std::optional<ResourceBase::State>& color_state,
                                         const std::optional<ResourceBase::State>& depth_state,
                                         Ptr<ResourceBase::Barriers>& transition_barriers_ptr,
                                         RenderCommandListBase& render_command_list)
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
        TextureBase* p_depth_texture = GetDepthAttachmentTexture();
        if (p_depth_texture)
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
        META_CHECK_ARG_NOT_NULL_DESCR(color_attach.texture_ptr, "Can not use color attachment without texture.");
        m_color_attachment_textures.push_back(static_cast<TextureBase&>(*color_attach.texture_ptr));
    }
    return m_color_attachment_textures;
}

TextureBase* RenderPassBase::GetDepthAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (!m_p_depth_attachment_texture && m_settings.depth_attachment.texture_ptr)
    {
        m_p_depth_attachment_texture = static_cast<TextureBase*>(m_settings.depth_attachment.texture_ptr.get());
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
        META_CHECK_ARG_NOT_NULL_DESCR(color_attach.texture_ptr, "Can not use color attachment without texture.");

        Ptr<TextureBase> color_attachment_ptr = std::static_pointer_cast<TextureBase>(color_attach.texture_ptr);
        if (color_attachment_ptr->GetSettings().type == Texture::Type::FrameBuffer)
            continue;

        m_non_frame_buffer_attachment_textures.emplace_back(std::move(color_attachment_ptr));
    }

    if (m_settings.depth_attachment.texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(std::static_pointer_cast<TextureBase>(m_settings.depth_attachment.texture_ptr));
    }

    if (m_settings.stencil_attachment.texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(std::static_pointer_cast<TextureBase>(m_settings.stencil_attachment.texture_ptr));
    }

    return m_non_frame_buffer_attachment_textures;
}

} // namespace Methane::Graphics
