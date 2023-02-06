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

FILE: Methane/Graphics/Vulkan/RenderPattern.cpp
Vulkan implementation of the render pattern interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/RenderPattern.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/RenderPass.h>
#include <Methane/Graphics/Vulkan/Types.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Vulkan
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

static vk::AttachmentLoadOp GetVulkanAttachmentLoadOp(Rhi::IRenderPattern::Attachment::LoadAction attachment_load_action)
{
    META_FUNCTION_TASK();
    using LoadAction = Rhi::IRenderPattern::Attachment::LoadAction;
    switch(attachment_load_action)
    {
    case LoadAction::DontCare:  return vk::AttachmentLoadOp::eDontCare;
    case LoadAction::Load:      return vk::AttachmentLoadOp::eLoad;
    case LoadAction::Clear:     return vk::AttachmentLoadOp::eClear;
    default: META_UNEXPECTED_ARG_RETURN(attachment_load_action, vk::AttachmentLoadOp::eDontCare);
    }
}

static vk::AttachmentStoreOp GetVulkanAttachmentStoreOp(Rhi::IRenderPattern::Attachment::StoreAction attachment_store_action)
{
    META_FUNCTION_TASK();
    using StoreAction = Rhi::IRenderPattern::Attachment::StoreAction;
    switch(attachment_store_action)
    {
    case StoreAction::DontCare:  return vk::AttachmentStoreOp::eDontCare;
    case StoreAction::Store:     return vk::AttachmentStoreOp::eStore;
    case StoreAction::Resolve:   return vk::AttachmentStoreOp::eNoneQCOM; // ?
    default: META_UNEXPECTED_ARG_RETURN(attachment_store_action, vk::AttachmentStoreOp::eDontCare);
    }
}

static vk::ImageLayout GetFinalImageLayoutOfAttachment(const Rhi::IRenderPattern::Attachment& attachment, bool is_final_pass)
{
    META_FUNCTION_TASK();
    const Rhi::IRenderPattern::Attachment::Type attachment_type = attachment.GetType();
    switch(attachment_type)
    {
    case Rhi::IRenderPattern::Attachment::Type::Color:   return is_final_pass
                                                              ? vk::ImageLayout::ePresentSrcKHR
                                                              : vk::ImageLayout::eColorAttachmentOptimal;
    case Rhi::IRenderPattern::Attachment::Type::Depth:   return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case Rhi::IRenderPattern::Attachment::Type::Stencil: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    default:
        META_UNEXPECTED_ARG_DESCR_RETURN(attachment_type, vk::ImageLayout::eUndefined,
                                         "attachment type is not supported by render pass");
    }
}

static vk::AttachmentDescription GetVulkanAttachmentDescription(const Rhi::IRenderPattern::Attachment& attachment, bool is_final_pass)
{
    META_FUNCTION_TASK();
    // FIXME: Current solution is unreliable, instead initial attachment State should be set in Rhi::IRenderPattern::Settings
    const vk::ImageLayout attachment_type_layout = attachment.GetType() == Rhi::IRenderPattern::Attachment::Type::Color
                                                 ? vk::ImageLayout::eColorAttachmentOptimal
                                                 : vk::ImageLayout::eDepthStencilAttachmentOptimal;
    const vk::ImageLayout initial_image_layout = attachment.load_action == Rhi::IRenderPattern::Attachment::LoadAction::Load
                                               ? attachment_type_layout
                                               : vk::ImageLayout::eUndefined;
    return vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags{},
        TypeConverter::PixelFormatToVulkan(attachment.format),
        GetVulkanSampleCountFlag(attachment.samples_count),
        GetVulkanAttachmentLoadOp(attachment.load_action),
        GetVulkanAttachmentStoreOp(attachment.store_action),
        // TODO: stencil is not supported yet
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        initial_image_layout,
        GetFinalImageLayoutOfAttachment(attachment, is_final_pass)
    );
}

static vk::UniqueRenderPass CreateVulkanRenderPass(const vk::Device& vk_device, const Rhi::IRenderPattern::Settings& settings)
{
    META_FUNCTION_TASK();

    std::vector<vk::AttachmentDescription> vk_attachment_descs;
    std::vector<vk::AttachmentReference> vk_color_attachment_refs;
    std::vector<vk::AttachmentReference> vk_input_attachment_refs;
    vk::AttachmentReference vk_depth_stencil_attachment_ref;

    for(const Rhi::IRenderPattern::ColorAttachment& color_attachment : settings.color_attachments)
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
        vk_depth_stencil_attachment_ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        vk_subpass_default.setPDepthStencilAttachment(&vk_depth_stencil_attachment_ref);
    }
    if (settings.stencil_attachment)
    {
        vk_attachment_descs.push_back(GetVulkanAttachmentDescription(*settings.stencil_attachment, settings.is_final_pass));
        vk_depth_stencil_attachment_ref.setAttachment(settings.stencil_attachment->attachment_index);
        vk_depth_stencil_attachment_ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
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

RenderPattern::RenderPattern(RenderContext& render_context, const Settings& settings)
    : Base::RenderPattern(render_context, settings)
    , m_vk_unique_render_pass(CreateVulkanRenderPass(render_context.GetVulkanDevice().GetNativeDevice(), settings))
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

Ptr<Rhi::IRenderPass> RenderPattern::CreateRenderPass(const Rhi::RenderPassSettings& settings)
{
    return std::make_shared<RenderPass>(*this, settings);
}

bool RenderPattern::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::RenderPattern::SetName(name))
        return false;

    SetVulkanObjectName(GetVulkanRenderContext().GetVulkanDevice().GetNativeDevice(), m_vk_unique_render_pass.get(), name);
    return true;
}

const RenderContext& RenderPattern::GetVulkanRenderContext() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const RenderContext&>(GetBaseRenderContext());
}

RenderContext& RenderPattern::GetVulkanRenderContext() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<RenderContext&>(GetBaseRenderContext());
}

} // namespace Methane::Graphics::Vulkan
