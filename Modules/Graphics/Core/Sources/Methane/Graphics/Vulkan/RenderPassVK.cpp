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

FILE: Methane/Graphics/Vulkan/RenderPassVK.cpp
Vulkan implementation of the render pass interface.

******************************************************************************/

#include "RenderPassVK.h"
#include "ContextVK.h"
#include "TextureVK.h"
#include "RenderContextVK.h"
#include "RenderCommandListVK.h"
#include "TypesVK.h"
#include "UtilsVK.hpp"

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

static vk::AttachmentDescription GetVulkanAttachmentDescription(const RenderPattern::Attachment& attachment, bool is_final_pass)
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
        vk::ImageLayout::eUndefined,
        is_final_pass ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eUndefined
    );
}

static vk::UniqueRenderPass CreateVulkanRenderPass(const vk::Device& vk_device, const RenderPattern::Settings& settings)
{
    META_FUNCTION_TASK();

    std::vector<vk::AttachmentDescription> vk_attachment_descs;
    std::vector<vk::AttachmentReference> vk_color_attachment_refs;
    std::vector<vk::AttachmentReference> vk_input_attachment_refs;
    vk::AttachmentReference vk_depth_stencil_attachment_ref;

    for(const RenderPattern::ColorAttachment& color_attachment : settings.color_attachments)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(color_attachment, settings.is_final_pass));
        vk_color_attachment_refs.emplace_back(color_attachment.attachment_index, vk::ImageLayout::eColorAttachmentOptimal);
    }

    std::vector<vk::SubpassDescription> vk_subpasses;
    vk::SubpassDescription& vk_subpass_default = vk_subpasses.emplace_back(
        vk::SubpassDescriptionFlags{},
        vk::PipelineBindPoint::eGraphics,
        vk_input_attachment_refs,
        vk_color_attachment_refs
    );

    std::vector<vk::SubpassDependency> vk_dependencies;
#if 0
    vk_dependencies.emplace_back(
        0, 0,
        vk::PipelineStageFlagBits{},
        vk::PipelineStageFlagBits{},
        vk::AccessFlagBits{},
        vk::AccessFlagBits{},
        vk::DependencyFlags{}
    );
#endif

    if (settings.depth_attachment)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(*settings.depth_attachment, settings.is_final_pass));
        vk_depth_stencil_attachment_ref.setAttachment(settings.depth_attachment->attachment_index);
        vk_depth_stencil_attachment_ref.setLayout(vk::ImageLayout::eDepthAttachmentOptimal);
        vk_subpass_default.setPDepthStencilAttachment(&vk_depth_stencil_attachment_ref);
    }
    if (settings.stencil_attachment)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(*settings.stencil_attachment, settings.is_final_pass));
        vk_depth_stencil_attachment_ref.setAttachment(settings.stencil_attachment->attachment_index);
        vk_depth_stencil_attachment_ref.setLayout(vk::ImageLayout::eStencilAttachmentOptimal);
        vk_subpass_default.setPDepthStencilAttachment(&vk_depth_stencil_attachment_ref);
    }

    return vk_device.createRenderPassUnique(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags{},
            vk_attachment_descs,
            vk_subpasses,
            vk_dependencies
        )
    );
}

static vk::UniqueFramebuffer CreateVulkanFrameBuffer(const vk::Device& vk_device, const vk::RenderPass& vk_render_pass, const RenderPass::Settings& settings)
{
    META_FUNCTION_TASK();
    std::vector<vk::ImageView> vk_attachment_views;
    std::transform(settings.attachments.begin(), settings.attachments.end(), std::back_inserter(vk_attachment_views),
        [](const Texture::Location& texture_location)
        {
            return dynamic_cast<FrameBufferTextureVK&>(texture_location.GetTexture()).GetNativeView();
        }
    );

    return vk_device.createFramebufferUnique(
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
    , m_vk_unique_render_pass(CreateVulkanRenderPass(render_context.GetDeviceVK().GetNativeDevice(), settings))
{
    META_FUNCTION_TASK();

    // Fill attachment clear colors
    m_attachment_clear_colors.reserve(GetAttachmentCount());
    std::transform(settings.color_attachments.begin(), settings.color_attachments.end(), std::back_inserter(m_attachment_clear_colors),
                   [](const ColorAttachment& color_attachment)
                   {
                       return vk::ClearValue(vk::ClearColorValue(color_attachment.clear_color.AsArray()));
                   });
    if (settings.depth_attachment || settings.stencil_attachment)
    {
        m_attachment_clear_colors.emplace_back(
            vk::ClearDepthStencilValue(
                settings.depth_attachment ? settings.depth_attachment->clear_value : 0.F,
                settings.stencil_attachment ? settings.stencil_attachment->clear_value : 0U
            )
        );
    }
}

void RenderPatternVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

    RenderPatternBase::SetName(name);
    SetVulkanObjectName(GetRenderContextVK().GetDeviceVK().GetNativeDevice(), m_vk_unique_render_pass.get(), name.c_str());
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
    , m_vk_unique_frame_buffer(CreateVulkanFrameBuffer(render_pattern.GetRenderContextVK().GetDeviceVK().GetNativeDevice(), render_pattern.GetNativeRenderPass(), settings))
    , m_vk_pass_begin_info(CreateBeginInfo(GetNativeFrameBuffer()))
{
    META_FUNCTION_TASK();
    render_pattern.GetRenderContextVK().Data::Emitter<IRenderContextVKCallback>::Connect(*this);
}

vk::RenderPassBeginInfo RenderPassVK::CreateBeginInfo(const vk::Framebuffer& vk_frame_buffer) const
{
    META_FUNCTION_TASK();
    const std::vector<vk::ClearValue>& attachment_clear_values = GetPatternVK().GetAttachmentClearValues();
    const FrameSize& frame_size = GetSettings().frame_size;
    return vk::RenderPassBeginInfo(
        GetPatternVK().GetNativeRenderPass(),
        vk_frame_buffer,
        vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(frame_size.GetWidth(), frame_size.GetHeight())),
        attachment_clear_values
    );
}

bool RenderPassVK::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (RenderPassBase::Update(settings))
    {
        Reset();
    }
    return false;
}

void RenderPassVK::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();

    m_vk_unique_frame_buffer.release();
    m_vk_pass_begin_info = vk::RenderPassBeginInfo();

    RenderPassBase::ReleaseAttachmentTextures();
}

void RenderPassVK::Begin(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();
    RenderPassBase::Begin(command_list);

    const vk::CommandBuffer& vk_command_buffer = static_cast<const RenderCommandListVK&>(command_list).GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary);
    vk_command_buffer.beginRenderPass(m_vk_pass_begin_info, vk::SubpassContents::eSecondaryCommandBuffers);
}

void RenderPassVK::End(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    const vk::CommandBuffer& vk_command_buffer = static_cast<const RenderCommandListVK&>(command_list).GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary);
    vk_command_buffer.endRenderPass();

    RenderPassBase::End(command_list);
}

void RenderPassVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

    RenderPassBase::SetName(name);
    SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), m_vk_unique_frame_buffer.get(), name.c_str());
}

void RenderPassVK::Reset()
{
    META_FUNCTION_TASK();
    m_vk_unique_frame_buffer = CreateVulkanFrameBuffer(GetContextVK().GetDeviceVK().GetNativeDevice(), GetPatternVK().GetNativeRenderPass(), GetSettings());
    m_vk_pass_begin_info = CreateBeginInfo(m_vk_unique_frame_buffer.get());
}

const IContextVK& RenderPassVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetPatternBase().GetRenderContextBase());
}

void RenderPassVK::OnRenderContextVKSwapchainChanged(RenderContextVK&)
{
    META_FUNCTION_TASK();
    for (const Texture::Location& texture_location : GetSettings().attachments)
    {
        dynamic_cast<FrameBufferTextureVK&>(texture_location.GetTexture()).ResetNativeImage();
    }
    Reset();
}

} // namespace Methane::Graphics
