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

FILE: Methane/Graphics/Metal/RenderPass.mm
Metal implementation of the render pass interface.

******************************************************************************/

#include <Methane/Graphics/Metal/RenderPass.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Graphics/Metal/Texture.hh>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IRenderPattern> IRenderPattern::Create(IRenderContext& render_context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Base::RenderPattern>(dynamic_cast<Base::RenderContext&>(render_context), settings);
}

Ptr<IRenderPass> IRenderPass::Create(IRenderPattern& render_pattern, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::RenderPass>(dynamic_cast<Base::RenderPattern&>(render_pattern), settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Metal
{

static MTLStoreAction GetLStoreAction(Rhi::RenderPassAttachment::StoreAction store_action)
{
    META_FUNCTION_TASK();
    switch(store_action)
    {
        case Rhi::RenderPassAttachment::StoreAction::DontCare: return MTLStoreActionDontCare;
        case Rhi::RenderPassAttachment::StoreAction::Store:    return MTLStoreActionStore;
        case Rhi::RenderPassAttachment::StoreAction::Resolve:  return MTLStoreActionMultisampleResolve;
        default: META_UNEXPECTED_ARG_RETURN(store_action, MTLStoreActionUnknown);
    }
}

static MTLLoadAction GetLLoadAction(Rhi::RenderPassAttachment::LoadAction load_action)
{
    META_FUNCTION_TASK();
    switch(load_action)
    {
        case Rhi::RenderPassAttachment::LoadAction::DontCare: return MTLLoadActionDontCare;
        case Rhi::RenderPassAttachment::LoadAction::Load:     return MTLLoadActionLoad;
        case Rhi::RenderPassAttachment::LoadAction::Clear:    return MTLLoadActionClear;
        default: META_UNEXPECTED_ARG_RETURN(load_action, MTLLoadActionDontCare);
    }
}

static void ConvertRenderPassAttachmentToMetal(const Base::RenderPass& render_pass, const Rhi::RenderPassAttachment& attachment,
                                               MTLRenderPassAttachmentDescriptor* mtl_attachment_desc)
{
    META_FUNCTION_TASK();
    const Rhi::ITexture::View& texture_location = render_pass.GetAttachmentTextureView(attachment);
    const Rhi::SubResource::Index& sub_resource_index = texture_location.GetSubresourceIndex();
    
    if (texture_location.GetTexture().GetSettings().type == Rhi::TextureType::FrameBuffer)
    {
        static_cast<Texture&>(texture_location.GetTexture()).UpdateFrameBuffer();
    }
    
    META_CHECK_ARG_NOT_NULL(mtl_attachment_desc);
    mtl_attachment_desc.texture       = static_cast<const Texture&>(texture_location.GetTexture()).GetNativeTexture();
    mtl_attachment_desc.level         = sub_resource_index.GetMipLevel();
    mtl_attachment_desc.loadAction    = GetLLoadAction(attachment.load_action);
    mtl_attachment_desc.storeAction   = GetLStoreAction(attachment.store_action);
    
    if (mtl_attachment_desc.texture.textureType == MTLTextureTypeCube ||
        mtl_attachment_desc.texture.textureType == MTLTextureTypeCubeArray)
    {
        mtl_attachment_desc.slice = sub_resource_index.GetArrayIndex() * 6U + sub_resource_index.GetDepthSlice();
    }
    else
    {
        mtl_attachment_desc.slice      = texture_location.GetSubresourceIndex().GetArrayIndex();
        mtl_attachment_desc.depthPlane = texture_location.GetSubresourceIndex().GetDepthSlice();
    }
}

RenderPass::RenderPass(Base::RenderPattern& render_pattern, const Settings& settings)
    : Base::RenderPass(render_pattern, settings)
{
    META_FUNCTION_TASK();
    Reset();
}

bool RenderPass::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    const bool settings_changed = Base::RenderPass::Update(settings);
    Reset();
    Data::Emitter<Rhi::IRenderPassCallback>::Emit(&Rhi::IRenderPassCallback::OnRenderPassUpdated, *this);
    return settings_changed;
}

void RenderPass::Reset()
{
    META_FUNCTION_TASK();

    m_mtl_pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    const IPattern::Settings& pattern_settings = GetBasePattern().GetSettings();

    uint32_t color_attach_index = 0;
    for(const ColorAttachment& color_attach : pattern_settings.color_attachments)
    {
        MTLRenderPassColorAttachmentDescriptor* mtl_color_attachment_desc = m_mtl_pass_descriptor.colorAttachments[color_attach_index++];
        ConvertRenderPassAttachmentToMetal(*this, color_attach, mtl_color_attachment_desc);
        mtl_color_attachment_desc.clearColor  = TypeConverter::ColorToMetalClearColor(color_attach.clear_color);
    }
    
    if (pattern_settings.depth_attachment)
    {
        ConvertRenderPassAttachmentToMetal(*this, *pattern_settings.depth_attachment, m_mtl_pass_descriptor.depthAttachment);
        m_mtl_pass_descriptor.depthAttachment.clearDepth = pattern_settings.depth_attachment->clear_value;
    }
    
    if (pattern_settings.stencil_attachment)
    {
        ConvertRenderPassAttachmentToMetal(*this, *pattern_settings.stencil_attachment, m_mtl_pass_descriptor.stencilAttachment);
        m_mtl_pass_descriptor.stencilAttachment.clearStencil  = pattern_settings.stencil_attachment->clear_value;
    }
}

MTLRenderPassDescriptor* RenderPass::GetNativeDescriptor(bool reset)
{
    META_FUNCTION_TASK();
    if (reset)
    {
        Reset();
    }
    return m_mtl_pass_descriptor;
}

const IContext& RenderPass::GetMetalContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const IContext&>(GetBasePattern().GetBaseRenderContext());
}

} // namespace Methane::Graphics::Metal
