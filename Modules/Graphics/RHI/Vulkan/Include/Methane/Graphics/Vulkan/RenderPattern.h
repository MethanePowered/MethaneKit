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

FILE: Methane/Graphics/Vulkan/RenderPattern.h
Vulkan implementation of the render pattern interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/RenderPattern.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

class RenderContext;

class RenderPattern
    : public Base::RenderPattern
{
public:
    RenderPattern(RenderContext& render_context, const Settings& settings);

    // IRenderPattern interface
    [[nodiscard]] Ptr<Rhi::IRenderPass> CreateRenderPass(const Rhi::RenderPassSettings& settings) override;

    // Base::Object overrides
    bool SetName(std::string_view name) override;

    [[nodiscard]] const RenderContext& GetVulkanRenderContext() const noexcept;
    [[nodiscard]] RenderContext&       GetVulkanRenderContext() noexcept;

    [[nodiscard]] const vk::RenderPass& GetNativeRenderPass() const noexcept                   { return m_vk_unique_render_pass.get(); }
    [[nodiscard]] const std::vector<vk::ClearValue>& GetAttachmentClearValues() const noexcept { return m_attachment_clear_colors; }

private:
    vk::UniqueRenderPass        m_vk_unique_render_pass;
    std::vector<vk::ClearValue> m_attachment_clear_colors;
};

} // namespace Methane::Graphics::Vulkan
