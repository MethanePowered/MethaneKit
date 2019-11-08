/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/RenderPassMT.mm
Metal implementation of the render pass interface.

******************************************************************************/

#include "RenderPassMT.hh"
#include "ContextMT.hh"
#include "TextureMT.hh"
#include "TypesMT.hh"

#include <Methane/Data/Instrumentation.h>

namespace Methane::Graphics
{

MTLStoreAction GetMTLStoreAction(RenderPass::Attachment::StoreAction store_action) noexcept
{
    ITT_FUNCTION_TASK();

    switch(store_action)
    {
        case RenderPass::Attachment::StoreAction::DontCare:   return MTLStoreActionDontCare;
        case RenderPass::Attachment::StoreAction::Store:      return MTLStoreActionStore;
        case RenderPass::Attachment::StoreAction::Unknown:    return MTLStoreActionUnknown;
    }
    return MTLStoreActionDontCare;
}

MTLLoadAction GetMTLLoadAction(RenderPass::Attachment::LoadAction load_action) noexcept
{
    ITT_FUNCTION_TASK();

    switch(load_action)
    {
        case RenderPass::Attachment::LoadAction::DontCare:    return MTLLoadActionDontCare;
        case RenderPass::Attachment::LoadAction::Load:        return MTLLoadActionLoad;
        case RenderPass::Attachment::LoadAction::Clear:       return MTLLoadActionClear;
    }
    return MTLLoadActionDontCare;
}

RenderPass::Ptr RenderPass::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderPassMT>(static_cast<ContextBase&>(context), settings);
}

RenderPassMT::RenderPassMT(ContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    ITT_FUNCTION_TASK();

    Reset();
}

void RenderPassMT::Update(const Settings& settings)
{
    ITT_FUNCTION_TASK();

    m_settings = settings;

    Reset();
}

void RenderPassMT::Reset()
{
    ITT_FUNCTION_TASK();

    m_mtl_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    
    uint32_t color_attach_index = 0;
    for(ColorAttachment& color_attach : m_settings.color_attachments)
    {
        if (color_attach.wp_texture.expired())
        {
            throw std::invalid_argument("Can not use color attachment without texture.");
        }

        TextureMT& color_texture = static_cast<TextureMT&>(*color_attach.wp_texture.lock());
        if (color_texture.GetSettings().type == Texture::Type::FrameBuffer)
        {
            color_texture.UpdateFrameBuffer();
        }
        
        m_mtl_pass_descriptor.colorAttachments[color_attach_index].texture     = color_texture.GetNativeTexture();
        m_mtl_pass_descriptor.colorAttachments[color_attach_index].clearColor  = TypeConverterMT::ColorToMetalClearColor(color_attach.clear_color);
        m_mtl_pass_descriptor.colorAttachments[color_attach_index].loadAction  = GetMTLLoadAction(color_attach.load_action);
        m_mtl_pass_descriptor.colorAttachments[color_attach_index].storeAction = GetMTLStoreAction(color_attach.store_action);
        
        color_attach_index++;
    }
    
    if (!m_settings.depth_attachment.wp_texture.expired())
    {
        const TextureMT& depth_texture = static_cast<const TextureMT&>(*m_settings.depth_attachment.wp_texture.lock());
        m_mtl_pass_descriptor.depthAttachment.texture         = depth_texture.GetNativeTexture();
        m_mtl_pass_descriptor.depthAttachment.clearDepth      = m_settings.depth_attachment.clear_value;
        m_mtl_pass_descriptor.depthAttachment.loadAction      = GetMTLLoadAction(m_settings.depth_attachment.load_action);
        m_mtl_pass_descriptor.depthAttachment.storeAction     = GetMTLStoreAction(m_settings.depth_attachment.store_action);
    }
    
    if (!m_settings.stencil_attachment.wp_texture.expired())
    {
        const TextureMT& stencil_texture = static_cast<const TextureMT&>(*m_settings.stencil_attachment.wp_texture.lock());
        m_mtl_pass_descriptor.stencilAttachment.texture       = stencil_texture.GetNativeTexture();
        m_mtl_pass_descriptor.stencilAttachment.clearStencil  = m_settings.stencil_attachment.clear_value;
        m_mtl_pass_descriptor.stencilAttachment.loadAction    = GetMTLLoadAction(m_settings.stencil_attachment.load_action);
        m_mtl_pass_descriptor.stencilAttachment.storeAction   = GetMTLStoreAction(m_settings.stencil_attachment.store_action);
    }
}
    
MTLRenderPassDescriptor* RenderPassMT::GetNativeDescriptor(bool reset)
{
    ITT_FUNCTION_TASK();
    
    if (reset)
    {
        Reset();
    }
    return m_mtl_pass_descriptor;
}

ContextMT& RenderPassMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextMT&>(m_context);
}

} // namespace Methane::Graphics
