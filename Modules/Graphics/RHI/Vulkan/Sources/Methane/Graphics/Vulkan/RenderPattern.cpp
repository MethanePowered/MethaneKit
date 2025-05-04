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
    using enum vk::SampleCountFlagBits;
    case 1U:  return e1;
    case 2U:  return e2;
    case 4U:  return e4;
    case 8U:  return e8;
    case 16U: return e16;
    case 32U: return e32;
    case 64U: return e64;
    default: META_UNEXPECTED_RETURN_DESCR(samples_count, vk::SampleCountFlagBits::e1, "attachment samples count is not in supported set");
    }
}

static vk::AttachmentLoadOp GetVulkanAttachmentLoadOp(Rhi::IRenderPattern::Attachment::LoadAction attachment_load_action)
{
    META_FUNCTION_TASK();
    switch(attachment_load_action)
    {
    using enum Rhi::IRenderPattern::Attachment::LoadAction;
    using enum vk::AttachmentLoadOp;
    case DontCare:  return eDontCare;
    case Load:      return eLoad;
    case Clear:     return eClear;
    default: META_UNEXPECTED_RETURN(attachment_load_action, vk::AttachmentLoadOp::eDontCare);
    }
}

static vk::AttachmentStoreOp GetVulkanAttachmentStoreOp(Rhi::IRenderPattern::Attachment::StoreAction attachment_store_action)
{
    META_FUNCTION_TASK();
    switch(attachment_store_action)
    {
    using enum Rhi::IRenderPattern::Attachment::StoreAction;
    using enum vk::AttachmentStoreOp;
    case DontCare:  return eDontCare;
    case Store:     return eStore;
    case Resolve:   return eNoneQCOM; // ?
    default: META_UNEXPECTED_RETURN(attachment_store_action, vk::AttachmentStoreOp::eDontCare);
    }
}

static vk::ImageLayout GetFinalImageLayoutOfAttachment(const Rhi::IRenderPattern::Attachment& attachment, bool is_final_pass)
{
    META_FUNCTION_TASK();
    switch(attachment.GetType())
    {
    using enum Rhi::IRenderPattern::Attachment::Type;
    using enum vk::ImageLayout;
    case Color:   return is_final_pass ? ePresentSrcKHR : eColorAttachmentOptimal;
    case Depth:   return eDepthStencilAttachmentOptimal;
    case Stencil: return eDepthStencilAttachmentOptimal;
    default:
        META_UNEXPECTED_RETURN_DESCR(attachment.GetType(), vk::ImageLayout::eUndefined,
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
        TypeConverter::SampleCountToVulkan(attachment.samples_count),
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
    std::ranges::transform(settings.color_attachments, std::back_inserter(m_attachment_clear_colors),
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
