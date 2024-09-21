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

FILE: Methane/Graphics/Vulkan/RenderPass.cpp
Vulkan implementation of the render pass interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/RenderPass.h>
#include <Methane/Graphics/Vulkan/RenderPattern.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Texture.h>
#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/Types.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Vulkan
{

RenderPass::RenderPass(RenderPattern& render_pattern, const Settings& settings)
    : Base::RenderPass(render_pattern, settings)
    , m_vk_context(dynamic_cast<const IContext&>(render_pattern.GetBaseRenderContext()))
    , m_vk_unique_frame_buffer(CreateNativeFrameBuffer(render_pattern.GetVulkanRenderContext().GetVulkanDevice().GetNativeDevice(), render_pattern.GetNativeRenderPass(), settings))
    , m_vk_pass_begin_info(CreateNativeBeginInfo(GetNativeFrameBuffer()))
{
    META_FUNCTION_TASK();
    static_cast<Data::IEmitter<IRenderContextCallback>&>(render_pattern.GetVulkanRenderContext()).Connect(*this);
}

bool RenderPass::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!Base::RenderPass::Update(settings))
        return false;

    Reset();
    return true;
}

void RenderPass::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();

    m_vk_unique_frame_buffer.release();
    m_vk_pass_begin_info = vk::RenderPassBeginInfo();

    Base::RenderPass::ReleaseAttachmentTextures();
}

void RenderPass::Begin(Base::RenderCommandList& command_list)
{
    META_FUNCTION_TASK();
    Base::RenderPass::Begin(command_list);

    const vk::CommandBuffer& vk_command_buffer = static_cast<const RenderCommandList&>(command_list).GetNativeCommandBuffer(CommandBufferType::Primary);
    vk_command_buffer.beginRenderPass(m_vk_pass_begin_info, vk::SubpassContents::eSecondaryCommandBuffers);
}

void RenderPass::End(Base::RenderCommandList& command_list)
{
    META_FUNCTION_TASK();
    const vk::CommandBuffer& vk_command_buffer = static_cast<const RenderCommandList&>(command_list).GetNativeCommandBuffer(CommandBufferType::Primary);
    vk_command_buffer.endRenderPass();

    Base::RenderPass::End(command_list);
}

bool RenderPass::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::RenderPass::SetName(name))
        return false;

    SetVulkanObjectName(GetVulkanContext().GetVulkanDevice().GetNativeDevice(), m_vk_unique_frame_buffer.get(), name);
    return true;
}

void RenderPass::Reset()
{
    META_FUNCTION_TASK();
    m_attachment_views.clear();
    m_vk_unique_frame_buffer = CreateNativeFrameBuffer(GetVulkanContext().GetVulkanDevice().GetNativeDevice(), GetVulkanPattern().GetNativeRenderPass(), GetSettings());
    m_vk_pass_begin_info = CreateNativeBeginInfo(m_vk_unique_frame_buffer.get());

    Data::Emitter<Rhi::IRenderPassCallback>::Emit(&Rhi::IRenderPassCallback::OnRenderPassUpdated, *this);
}

RenderPattern& RenderPass::GetVulkanPattern() const noexcept
{
    return static_cast<RenderPattern&>(GetBasePattern());
}

void RenderPass::OnRenderContextSwapchainChanged(RenderContext&)
{
    META_FUNCTION_TASK();
    const Rhi::TextureViews& attachment_texture_locations = GetSettings().attachments;
    if (attachment_texture_locations.empty())
        return;

    for (const Rhi::TextureView& texture_location : attachment_texture_locations)
    {
        if (texture_location.GetTexture().GetSettings().type == Rhi::TextureType::FrameBuffer)
            static_cast<Texture&>(texture_location.GetTexture()).ResetNativeFrameImage();
    }

    Reset();
}

const ResourceView& RenderPass::GetVulkanAttachmentTextureView(const Attachment& attachment) const
{
    META_FUNCTION_TASK();
    META_CHECK_LESS_DESCR(attachment.attachment_index, m_attachment_views.size(),
                              "attachment index is out of bounds of render pass VK attachments array");
    return m_attachment_views[attachment.attachment_index];
}

vk::RenderPassBeginInfo RenderPass::CreateNativeBeginInfo(const vk::Framebuffer& vk_frame_buffer) const
{
    META_FUNCTION_TASK();
    const std::vector<vk::ClearValue>& attachment_clear_values = GetVulkanPattern().GetAttachmentClearValues();
    const FrameSize& frame_size = GetSettings().frame_size;
    return vk::RenderPassBeginInfo(
        GetVulkanPattern().GetNativeRenderPass(),
        vk_frame_buffer,
        vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(frame_size.GetWidth(), frame_size.GetHeight())),
        attachment_clear_values
    );
}

vk::UniqueFramebuffer RenderPass::CreateNativeFrameBuffer(const vk::Device& vk_device, const vk::RenderPass& vk_render_pass, const Settings& settings)
{
    META_FUNCTION_TASK();
    if (m_attachment_views.empty())
    {
        std::transform(settings.attachments.begin(), settings.attachments.end(), std::back_inserter(m_attachment_views),
                       [](const Rhi::TextureView& texture_location)
                       { return Vulkan::ResourceView(texture_location, Rhi::ResourceUsageMask(Rhi::ResourceUsage::RenderTarget)); });
    }

    std::vector<vk::ImageView> vk_attachment_views;
    std::transform(m_attachment_views.begin(), m_attachment_views.end(), std::back_inserter(vk_attachment_views),
                   [](const ResourceView& resource_view)
                   { return resource_view.GetNativeImageView(); });

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

} // namespace Methane::Graphics::Vulkan
