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

FILE: Methane/Graphics/Vulkan/RenderPassVK.h
Vulkan implementation of the render pass interface.

******************************************************************************/

#pragma once

#include "RenderContextVK.h"
#include "ResourceVK.h"

#include <Methane/Graphics/Base/RenderPass.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct IContextVK;
class RenderContextVK;

class RenderPatternVK
    : public Base::RenderPattern
{
public:
    RenderPatternVK(RenderContextVK& render_context, const Settings& settings);

    // Base::Object overrides
    bool SetName(const std::string& name) override;

    [[nodiscard]] const RenderContextVK& GetRenderContextVK() const noexcept;
    [[nodiscard]] RenderContextVK&       GetRenderContextVK() noexcept;

    [[nodiscard]] const vk::RenderPass& GetNativeRenderPass() const noexcept                   { return m_vk_unique_render_pass.get(); }
    [[nodiscard]] const std::vector<vk::ClearValue>& GetAttachmentClearValues() const noexcept { return m_attachment_clear_colors; }

private:
    vk::UniqueRenderPass        m_vk_unique_render_pass;
    std::vector<vk::ClearValue> m_attachment_clear_colors;
};

class RenderPassVK final
    : public Base::RenderPass
    , protected Data::Receiver<IRenderContextVKCallback>
{
public:
    RenderPassVK(RenderPatternVK& render_pattern, const Settings& settings);

    // IRenderPass interface
    bool Update(const Settings& settings) override;
    void ReleaseAttachmentTextures() override;

    // Base::RenderPass overrides
    void Begin(Base::RenderCommandList& command_list) override;
    void End(Base::RenderCommandList& command_list) override;

    // Base::Object overrides
    bool SetName(const std::string& name) override;
    
    void Reset();

    const IContextVK& GetContextVK() const noexcept { return m_vk_context; }
    RenderPatternVK&  GetPatternVK() const noexcept { return static_cast<RenderPatternVK&>(GetPatternBase()); }

    const vk::Framebuffer& GetNativeFrameBuffer() const noexcept { return m_vk_unique_frame_buffer.get(); }

private:
    // IRenderContextVKCallback overrides
    void OnRenderContextVKSwapchainChanged(RenderContextVK&) override;

    const ResourceViewVK&   GetAttachmentTextureViewVK(const Attachment& attachment) const;
    vk::RenderPassBeginInfo CreateNativeBeginInfo(const vk::Framebuffer& vk_frame_buffer) const;
    vk::UniqueFramebuffer   CreateNativeFrameBuffer(const vk::Device& vk_device, const vk::RenderPass& vk_render_pass, const Settings& settings);

    const IContextVK&       m_vk_context;
    ResourceViewsVK         m_vk_attachments;
    vk::UniqueFramebuffer   m_vk_unique_frame_buffer;
    vk::RenderPassBeginInfo m_vk_pass_begin_info;
};

} // namespace Methane::Graphics
