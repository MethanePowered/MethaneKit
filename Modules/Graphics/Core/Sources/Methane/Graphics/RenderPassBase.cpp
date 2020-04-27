/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

bool RenderPass::Attachment::operator==(const RenderPass::Attachment& other) const
{
    META_FUNCTION_TASK();
    return ( (  wp_texture.expired() &&  other.wp_texture.expired() ) ||
             ( !wp_texture.expired() && !other.wp_texture.expired() &&
              std::addressof(*wp_texture.lock()) == std::addressof(*other.wp_texture.lock()) ) ) &&
           level == other.level &&
           slice == other.slice &&
           depth_plane == other.depth_plane &&
           load_action == other.load_action &&
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
           shader_access_mask == other.shader_access_mask;
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
}

bool RenderPassBase::Update(const RenderPass::Settings& settings)
{
    META_FUNCTION_TASK();
    if (m_settings == settings)
        return false;

    m_settings = settings;
    m_color_attach_resources.clear();
    m_sp_color_begin_transition_barriers.reset();
    m_sp_color_end_transition_barriers.reset();
    m_sp_attach_resources_transition_barriers.reset();
    return true;
}

void RenderPassBase::Begin(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    if (m_is_begun)
    {
        throw std::logic_error("Can not begin pass which was begun already and was not ended.");
    }

    if (!m_sp_attach_resources_transition_barriers)
    {
        for (ColorAttachment& color_attachment : m_settings.color_attachments)
        {
            Ptr<Texture> sp_color_texture = color_attachment.wp_texture.lock();
            if (!sp_color_texture)
                continue;

            TextureBase& color_texture = dynamic_cast<TextureBase&>(*sp_color_texture);
            color_texture.SetState(ResourceBase::State::RenderTarget, m_sp_attach_resources_transition_barriers);
        }

        Ptr<Texture> sp_depth_texture = m_settings.depth_attachment.wp_texture.lock();
        if (sp_depth_texture)
        {
            TextureBase& depth_texture = dynamic_cast<TextureBase&>(*sp_depth_texture);
            depth_texture.SetState(ResourceBase::State::DepthWrite, m_sp_attach_resources_transition_barriers);
        }
    }

    if (m_sp_attach_resources_transition_barriers && !m_sp_attach_resources_transition_barriers->IsEmpty())
    {
        command_list.SetResourceBarriers(*m_sp_attach_resources_transition_barriers);
    }

    m_is_begun = true;
}

void RenderPassBase::End(RenderCommandListBase&)
{
    META_FUNCTION_TASK();

    if (!m_is_begun)
    {
        throw std::logic_error("Can not end render pass, which was not begun.");
    }

    m_is_begun = false;
}

const Refs<Resource>& RenderPassBase::GetColorAttachmentResources() const
{
    META_FUNCTION_TASK();
    if (!m_color_attach_resources.empty())
        return m_color_attach_resources;

    m_color_attach_resources.reserve(m_settings.color_attachments.size());
    for (const ColorAttachment& color_attach : m_settings.color_attachments)
    {
        if (color_attach.wp_texture.expired())
        {
            throw std::invalid_argument("Can not use color attachment without texture.");
        }
        m_color_attach_resources.push_back(*color_attach.wp_texture.lock());
    }
    return m_color_attach_resources;
}

const ResourceBase::Barriers& RenderPassBase::GetColorBeginTransitionBarriers() const noexcept
{
    META_FUNCTION_TASK();
    if (m_sp_color_begin_transition_barriers)
        return *m_sp_color_begin_transition_barriers;

    m_sp_color_begin_transition_barriers = ResourceBase::Barriers::CreateTransition(GetColorAttachmentResources(),
                                              ResourceBase::State::Present, ResourceBase::State::RenderTarget);
    return *m_sp_color_begin_transition_barriers;
}

const ResourceBase::Barriers& RenderPassBase::GetColorEndTransitionBarriers() const noexcept
{
    META_FUNCTION_TASK();
    if (m_sp_color_end_transition_barriers)
        return *m_sp_color_end_transition_barriers;

    m_sp_color_end_transition_barriers = ResourceBase::Barriers::CreateTransition(GetColorAttachmentResources(),
                                            ResourceBase::State::RenderTarget, ResourceBase::State::Present);
    return *m_sp_color_end_transition_barriers;
}

} // namespace Methane::Graphics
