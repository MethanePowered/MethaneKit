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

#include <Methane/Graphics/RenderPassBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct IContextVK;
class RenderContextVK;

class RenderPatternVK
    : public RenderPatternBase
{
public:
    RenderPatternVK(RenderContextVK& render_context, const Settings& settings);
    ~RenderPatternVK(); // NOSONAR

    [[nodiscard]] const RenderContextVK& GetRenderContextVK() const noexcept;
    [[nodiscard]] RenderContextVK&       GetRenderContextVK() noexcept;

    [[nodiscard]] const vk::RenderPass& GetNativeRenderPass() const noexcept { return m_vk_render_pass; }

private:
    vk::RenderPass m_vk_render_pass;
};

class RenderPassVK final : public RenderPassBase
{
public:
    RenderPassVK(RenderPatternVK& render_pattern, const Settings& settings);
    ~RenderPassVK(); // NOSONAR

    // RenderPass interface
    bool Update(const Settings& settings) override;
    
    void Reset();

    const IContextVK& GetContextVK() const noexcept;
    RenderPatternVK&  GetPatternVK() const noexcept { return static_cast<RenderPatternVK&>(GetPatternBase()); }

private:
    vk::Framebuffer m_vk_frame_buffer;
};

} // namespace Methane::Graphics
