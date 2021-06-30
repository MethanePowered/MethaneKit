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

FILE: Methane/Graphics/Vulkan/RenderPassVK.mm
Vulkan implementation of the render pass interface.

******************************************************************************/

#include "RenderPassVK.h"
#include "ContextVK.h"
#include "TextureVK.h"
#include "RenderContextVK.h"
#include "TypesVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

vk::SampleCountFlagBits GetVulkanSampleCountFlag(Data::Size samples_count)
{
    META_FUNCTION_TASK();
    switch(samples_count)
    {
    case 1U:  return vk::SampleCountFlagBits::e1;
    case 2U:  return vk::SampleCountFlagBits::e2;
    case 4U:  return vk::SampleCountFlagBits::e4;
    case 8U:  return vk::SampleCountFlagBits::e8;
    case 16U: return vk::SampleCountFlagBits::e16;
    case 32U: return vk::SampleCountFlagBits::e32;
    case 64U: return vk::SampleCountFlagBits::e64;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(samples_count, vk::SampleCountFlagBits::e1, "attachment samples count is not in supported set");
    }
}

static vk::AttachmentLoadOp GetVulkanAttachmentLoadOp(RenderPattern::Attachment::LoadAction attachment_load_action)
{
    META_FUNCTION_TASK();
    using LoadAction = RenderPattern::Attachment::LoadAction;
    switch(attachment_load_action)
    {
    case LoadAction::DontCare:  return vk::AttachmentLoadOp::eDontCare;
    case LoadAction::Load:      return vk::AttachmentLoadOp::eLoad;
    case LoadAction::Clear:     return vk::AttachmentLoadOp::eClear;
    default: META_UNEXPECTED_ARG_RETURN(attachment_load_action, vk::AttachmentLoadOp::eDontCare);
    }
}

static vk::AttachmentStoreOp GetVulkanAttachmentStoreOp(RenderPattern::Attachment::StoreAction attachment_store_action)
{
    META_FUNCTION_TASK();
    using StoreAction = RenderPattern::Attachment::StoreAction;
    switch(attachment_store_action)
    {
    case StoreAction::DontCare:  return vk::AttachmentStoreOp::eDontCare;
    case StoreAction::Store:     return vk::AttachmentStoreOp::eStore;
    case StoreAction::Resolve:   return vk::AttachmentStoreOp::eNoneQCOM; // ?
    default: META_UNEXPECTED_ARG_RETURN(attachment_store_action, vk::AttachmentStoreOp::eDontCare);
    }
}

static vk::AttachmentDescription GetVulkanAttachmentDescription(const RenderPattern::Attachment& attachment)
{
    META_FUNCTION_TASK();
    return vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags{},
        TypeConverterVK::PixelFormatToVulkan(attachment.format),
        GetVulkanSampleCountFlag(attachment.samples_count),
        GetVulkanAttachmentLoadOp(attachment.load_action),
        GetVulkanAttachmentStoreOp(attachment.store_action),
        vk::AttachmentLoadOp::eDontCare,  // TODO: stencil is not supported yet
        vk::AttachmentStoreOp::eDontCare, // TODO: stencil is not supported yet
        vk::ImageLayout::eUndefined,      // TODO: add initial resource state in render pattern attachment
        vk::ImageLayout::ePresentSrcKHR   // TODO: add final resource state in render pattern attachment
    );
}

static vk::RenderPass CreateVulkanRenderPass(const vk::Device& vk_device, const RenderPattern::Settings& settings)
{
    META_FUNCTION_TASK();

    std::vector<vk::AttachmentDescription> vk_attachment_descs;
    std::vector<vk::AttachmentReference> vk_color_attachment_refs;
    std::vector<vk::AttachmentReference> vk_input_attachment_refs;
    vk::AttachmentReference vk_depth_stencil_attachment_ref;

    for(const RenderPattern::ColorAttachment& color_attachment : settings.color_attachments)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(color_attachment));
        vk_color_attachment_refs.emplace_back(color_attachment.attachment_index, vk::ImageLayout::eColorAttachmentOptimal);
    }

    std::vector<vk::SubpassDescription> vk_subpasses;
    vk::SubpassDescription& vk_subpass_default = vk_subpasses.emplace_back(
        vk::SubpassDescriptionFlags{},
        vk::PipelineBindPoint::eGraphics,
        vk_input_attachment_refs,
        vk_color_attachment_refs
    );

    if (settings.depth_attachment)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(*settings.depth_attachment));
        vk_depth_stencil_attachment_ref.setAttachment(settings.depth_attachment->attachment_index);
        vk_depth_stencil_attachment_ref.setLayout(vk::ImageLayout::eDepthAttachmentOptimal);
        vk_subpass_default.setPDepthStencilAttachment(&vk_depth_stencil_attachment_ref);
    }
    if (settings.stencil_attachment)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(*settings.stencil_attachment));
        vk_depth_stencil_attachment_ref.setAttachment(settings.stencil_attachment->attachment_index);
        vk_depth_stencil_attachment_ref.setLayout(vk::ImageLayout::eStencilAttachmentOptimal);
        vk_subpass_default.setPDepthStencilAttachment(&vk_depth_stencil_attachment_ref);
    }

    return vk_device.createRenderPass(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags{},
            vk_attachment_descs,
            vk_subpasses
        )
    );
}

static vk::Framebuffer CreateVulkanFrameBuffer(const vk::Device& vk_device, const vk::RenderPass& vk_render_pass, const RenderPass::Settings& settings)
{
    META_FUNCTION_TASK();
    std::vector<vk::ImageView> vk_attachment_views;
    std::transform(settings.attachments.begin(), settings.attachments.end(), std::back_inserter(vk_attachment_views),
        [](const Texture::Location& texture_location)
        {
            return dynamic_cast<FrameBufferTextureVK&>(texture_location.GetTexture()).GetNativeImageView();
        }
    );

    return vk_device.createFramebuffer(
        vk::FramebufferCreateInfo(
            vk::FramebufferCreateFlags{},
            vk_render_pass,
            vk_attachment_views,
            settings.frame_size.GetWidth(),
            settings.frame_size.GetHeight(),
            1U
        )
    );
}

Ptr<RenderPattern> RenderPattern::Create(RenderContext& render_context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPatternVK>(dynamic_cast<RenderContextVK&>(render_context), settings);
}

RenderPatternVK::RenderPatternVK(RenderContextVK& render_context, const Settings& settings)
    : RenderPatternBase(render_context, settings)
    , m_vk_render_pass(CreateVulkanRenderPass(render_context.GetDeviceVK().GetNativeDevice(), settings))
{
    META_FUNCTION_TASK();
}

RenderPatternVK::~RenderPatternVK()
{
    META_FUNCTION_TASK();
    GetRenderContextVK().GetDeviceVK().GetNativeDevice().destroy(m_vk_render_pass);
}

const RenderContextVK& RenderPatternVK::GetRenderContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const RenderContextVK&>(GetRenderContextBase());
}

RenderContextVK& RenderPatternVK::GetRenderContextVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<RenderContextVK&>(GetRenderContextBase());
}

Ptr<RenderPass> RenderPass::Create(RenderPattern& render_pattern, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPassVK>(dynamic_cast<RenderPatternVK&>(render_pattern), settings);
}

RenderPassVK::RenderPassVK(RenderPatternVK& render_pattern, const Settings& settings)
    : RenderPassBase(render_pattern, settings)
    , m_vk_frame_buffer(CreateVulkanFrameBuffer(render_pattern.GetRenderContextVK().GetDeviceVK().GetNativeDevice(), render_pattern.GetNativeRenderPass(), settings))
{
    META_FUNCTION_TASK();
}

RenderPassVK::~RenderPassVK()
{
    META_FUNCTION_TASK();
    GetContextVK().GetDeviceVK().GetNativeDevice().destroy(m_vk_frame_buffer);
}

bool RenderPassVK::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (RenderPassBase::Update(settings))
    {
        Reset();
        return true;
    }
    return false;
}

void RenderPassVK::Reset()
{
    META_FUNCTION_TASK();
    GetContextVK().GetDeviceVK().GetNativeDevice().destroy(m_vk_frame_buffer);
    m_vk_frame_buffer = CreateVulkanFrameBuffer(GetContextVK().GetDeviceVK().GetNativeDevice(), GetPatternVK().GetNativeRenderPass(), GetSettings());
}

const IContextVK& RenderPassVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetPatternBase().GetRenderContextBase());
}

} // namespace Methane::Graphics
