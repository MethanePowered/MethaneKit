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

FILE: Methane/Graphics/Base/RenderPass.cpp
Base implementation of the render pass interface.

******************************************************************************/

#include <Methane/Graphics/Base/RenderPass.h>
#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Graphics/Base/Texture.h>
#include <Methane/Graphics/Base/RenderCommandList.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Base
{

RenderPass::RenderPass(RenderPattern& render_pattern, const Settings& settings, bool update_attachment_states)
    : m_pattern_base_ptr(render_pattern.GetDerivedPtr<RenderPattern>())
    , m_settings(settings)
    , m_update_attachment_states(update_attachment_states)
{
    META_FUNCTION_TASK();
    InitAttachmentStates();
}

bool RenderPass::Update(const Rhi::RenderPassSettings& settings)
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

void RenderPass::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();
    m_non_frame_buffer_attachment_textures.clear();
    m_settings.attachments.clear();
}

void RenderPass::Begin(RenderCommandList&)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE_DESCR(m_is_begun, "can not begin pass which was begun already and was not ended");

    if (m_update_attachment_states)
    {
        SetAttachmentStates(Rhi::ResourceState::RenderTarget, Rhi::ResourceState::DepthWrite);
    }
    m_is_begun = true;
}

void RenderPass::End(RenderCommandList&)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(m_is_begun, "can not end render pass, which was not begun");

    if (m_update_attachment_states && GetPattern().GetSettings().is_final_pass)
    {
        SetAttachmentStates(Rhi::ResourceState::Present, { });
    }
    m_is_begun = false;
}

void RenderPass::InitAttachmentStates() const
{
    META_FUNCTION_TASK();
    const bool             is_final_pass          = GetPattern().GetSettings().is_final_pass;
    const Rhi::ResourceState color_attachment_state = is_final_pass ? Rhi::ResourceState::Present : Rhi::ResourceState::RenderTarget;
    for (const Ref<Texture>& color_texture_ref : GetColorAttachmentTextures())
    {
        if (color_texture_ref.get().GetState() == Rhi::ResourceState::Common ||
            color_texture_ref.get().GetState() == Rhi::ResourceState::Undefined)
            color_texture_ref.get().SetState(color_attachment_state);
    }
}

void RenderPass::SetAttachmentStates(const Opt<Rhi::ResourceState>& color_state,
                                     const Opt<Rhi::ResourceState>& depth_state) const
{
    META_FUNCTION_TASK();
    if (color_state)
    {
        for (const Ref<Texture>& color_texture_ref : GetColorAttachmentTextures())
        {
            color_texture_ref.get().SetState(*color_state);
        }
    }

    if (depth_state)
    {
        if (Texture* p_depth_texture = GetDepthAttachmentTexture();
            p_depth_texture)
        {
            p_depth_texture->SetState(*depth_state);
        }
    }
}

void RenderPass::SetAttachmentStates(const Opt<Rhi::ResourceState>& color_state,
                                     const Opt<Rhi::ResourceState>& depth_state,
                                     Ptr<Rhi::IResourceBarriers>& transition_barriers_ptr,
                                     RenderCommandList& render_command_list) const
{
    META_FUNCTION_TASK();
    bool attachment_states_changed = false;

    if (color_state)
    {
        for (const Ref<Texture>& color_texture_ref : GetColorAttachmentTextures())
        {
            attachment_states_changed |= color_texture_ref.get().SetState(*color_state, transition_barriers_ptr);
        }
    }

    if (depth_state)
    {
        if (Texture* p_depth_texture = GetDepthAttachmentTexture();
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

const Rhi::ITexture::View& RenderPass::GetAttachmentTextureView(const Rhi::RenderPassAttachment& attachment) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(attachment.attachment_index, m_settings.attachments.size(),
                              "attachment index is out of bounds of render pass attachments array");
    return m_settings.attachments[attachment.attachment_index];
}

const Refs<Texture>& RenderPass::GetColorAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_color_attachment_textures.empty())
        return m_color_attachment_textures;

    const ColorAttachments& color_attachments = GetPattern().GetSettings().color_attachments;
    m_color_attachment_textures.reserve(color_attachments.size());
    for (const ColorAttachment& color_attach : color_attachments)
    {
        m_color_attachment_textures.push_back(static_cast<Texture&>(GetAttachmentTextureView(color_attach).GetTexture()));
    }
    return m_color_attachment_textures;
}

Texture* RenderPass::GetDepthAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (m_p_depth_attachment_texture)
        return m_p_depth_attachment_texture;

    const Opt<Rhi::RenderPassDepthAttachment>& depth_attachment_opt = GetPattern().GetSettings().depth_attachment;
    if (!depth_attachment_opt)
        return nullptr;

    m_p_depth_attachment_texture = static_cast<Texture*>(GetAttachmentTextureView(*depth_attachment_opt).GetTexturePtr().get());
    return m_p_depth_attachment_texture;
}

Texture* RenderPass::GetStencilAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (m_p_stencil_attachment_texture)
        return m_p_stencil_attachment_texture;

    const Opt<StencilAttachment>& stencil_attachment_opt = GetPattern().GetSettings().stencil_attachment;
    if (!stencil_attachment_opt)
        return nullptr;

    m_p_stencil_attachment_texture = static_cast<Texture*>(GetAttachmentTextureView(*stencil_attachment_opt).GetTexturePtr().get());
    return m_p_stencil_attachment_texture;
}

const Ptrs<Texture>& RenderPass::GetNonFrameBufferAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_non_frame_buffer_attachment_textures.empty())
        return m_non_frame_buffer_attachment_textures;

    m_non_frame_buffer_attachment_textures.reserve(m_settings.attachments.size());

    for (const Ref<Texture>& color_texture_ref : GetColorAttachmentTextures())
    {
        Ptr<Texture> color_attachment_ptr = color_texture_ref.get().GetPtr<Texture>();
        if (color_attachment_ptr->GetSettings().type == Rhi::ITexture::Type::FrameBuffer)
            continue;

        m_non_frame_buffer_attachment_textures.emplace_back(std::move(color_attachment_ptr));
    }

    if (Texture* depth_texture_ptr = GetDepthAttachmentTexture();
        depth_texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(depth_texture_ptr->GetPtr<Texture>());
    }

    if (Texture* stencil_texture_ptr = GetStencilAttachmentTexture();
        stencil_texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(stencil_texture_ptr->GetPtr<Texture>());
    }

    return m_non_frame_buffer_attachment_textures;
}

} // namespace Methane::Graphics::Base
