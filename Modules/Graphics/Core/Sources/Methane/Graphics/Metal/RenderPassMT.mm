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

FILE: Methane/Graphics/Metal/RenderPassMT.mm
Metal implementation of the render pass interface.

******************************************************************************/

#include "RenderPassMT.hh"
#include "RenderContextMT.hh"
#include "TextureMT.hh"
#include "TypesMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

static MTLStoreAction GetMTLStoreAction(RenderPass::Attachment::StoreAction store_action)
{
    META_FUNCTION_TASK();
    switch(store_action)
    {
        case RenderPass::Attachment::StoreAction::DontCare:   return MTLStoreActionDontCare;
        case RenderPass::Attachment::StoreAction::Store:      return MTLStoreActionStore;
        case RenderPass::Attachment::StoreAction::Resolve:    return MTLStoreActionMultisampleResolve;
        default:                                              META_UNEXPECTED_ARG_RETURN(store_action, MTLStoreActionUnknown);
    }
}

static MTLLoadAction GetMTLLoadAction(RenderPass::Attachment::LoadAction load_action)
{
    META_FUNCTION_TASK();
    switch(load_action)
    {
        case RenderPass::Attachment::LoadAction::DontCare:    return MTLLoadActionDontCare;
        case RenderPass::Attachment::LoadAction::Load:        return MTLLoadActionLoad;
        case RenderPass::Attachment::LoadAction::Clear:       return MTLLoadActionClear;
        default:                                              META_UNEXPECTED_ARG_RETURN(load_action, MTLLoadActionDontCare);
    }
}

static void ConvertRenderPassAttcachmentToMetal(const RenderPass::Attachment& pass_attachment, MTLRenderPassAttachmentDescriptor* mtl_attachment_desc)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(mtl_attachment_desc);
    mtl_attachment_desc.texture       = static_cast<const TextureMT&>(pass_attachment.texture_location.GetTexture()).GetNativeTexture();
    mtl_attachment_desc.slice         = pass_attachment.texture_location.GetSubresourceIndex().GetArrayIndex();
    mtl_attachment_desc.level         = pass_attachment.texture_location.GetSubresourceIndex().GetMipLevel();
    mtl_attachment_desc.depthPlane    = pass_attachment.texture_location.GetSubresourceIndex().GetDepthSlice();
    mtl_attachment_desc.loadAction    = GetMTLLoadAction(pass_attachment.load_action);
    mtl_attachment_desc.storeAction   = GetMTLStoreAction(pass_attachment.store_action);
}

Ptr<RenderPass> RenderPass::Create(const RenderContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPassMT>(dynamic_cast<const RenderContextBase&>(context), settings);
}

RenderPassMT::RenderPassMT(const RenderContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    META_FUNCTION_TASK();
    Reset();
}

bool RenderPassMT::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    const bool settings_changed = RenderPassBase::Update(settings);
    Reset();
    return settings_changed;
}

void RenderPassMT::Reset()
{
    META_FUNCTION_TASK();

    m_mtl_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    const Settings& settings = GetSettings();
    
    uint32_t color_attach_index = 0;
    for(const ColorAttachment& color_attach : settings.color_attachments)
    {
        META_CHECK_ARG_TRUE_DESCR(color_attach.texture_location.IsInitialized(), "can not use color attachment without texture");
        if (color_attach.texture_location.GetTexture().GetSettings().type == Texture::Type::FrameBuffer)
        {
            static_cast<TextureMT&>(color_attach.texture_location.GetTexture()).UpdateFrameBuffer();
        }

        MTLRenderPassColorAttachmentDescriptor* mtl_color_attachment_desc = m_mtl_pass_descriptor.colorAttachments[color_attach_index];
        ConvertRenderPassAttcachmentToMetal(color_attach, mtl_color_attachment_desc);
        mtl_color_attachment_desc.clearColor  = TypeConverterMT::ColorToMetalClearColor(color_attach.clear_color);
        
        color_attach_index++;
    }
    
    if (settings.depth_attachment.texture_location.IsInitialized())
    {
        ConvertRenderPassAttcachmentToMetal(settings.depth_attachment, m_mtl_pass_descriptor.depthAttachment);
        m_mtl_pass_descriptor.depthAttachment.clearDepth = settings.depth_attachment.clear_value;
    }
    
    if (settings.stencil_attachment.texture_location.IsInitialized())
    {
        ConvertRenderPassAttcachmentToMetal(settings.stencil_attachment, m_mtl_pass_descriptor.stencilAttachment);
        m_mtl_pass_descriptor.stencilAttachment.clearStencil  = settings.stencil_attachment.clear_value;
    }
}
    
MTLRenderPassDescriptor* RenderPassMT::GetNativeDescriptor(bool reset)
{
    META_FUNCTION_TASK();
    if (reset)
    {
        Reset();
    }
    return m_mtl_pass_descriptor;
}

const IContextMT& RenderPassMT::GetContextMT() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextMT&>(GetRenderContext());
}

} // namespace Methane::Graphics
