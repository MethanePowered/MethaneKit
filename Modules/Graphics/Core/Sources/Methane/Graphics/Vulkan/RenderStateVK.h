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

FILE: Methane/Graphics/Vulkan/RenderStateVK.h
Vulkan implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderStateBase.h>

#include <vector>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct IContextVK;

class ViewStateVK final : public ViewStateBase
{
public:
    explicit ViewStateVK(const Settings& settings);

    // IViewState overrides
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // ViewStateBase interface
    void Apply(RenderCommandListBase& command_list) override;

    const std::vector<vk::Viewport>& GetNativeViewports() const noexcept    { return m_vk_viewports; }
    const std::vector<vk::Rect2D>&   GetNativeScissorRects() const noexcept { return m_vk_scissor_rects; }

private:
    std::vector<vk::Viewport> m_vk_viewports;
    std::vector<vk::Rect2D>   m_vk_scissor_rects;
};

class RenderStateVK final : public RenderStateBase
{
public:
    RenderStateVK(const RenderContextBase& context, const Settings& settings);
    
    // IRenderState interface
    void Reset(const Settings& settings) override;

    // RenderStateBase interface
    void Apply(RenderCommandListBase& render_command_list, Groups state_groups) override;

    // IObject interface
    bool SetName(const std::string& name) override;

    const vk::Pipeline& GetNativePipeline() const noexcept { return m_vk_unique_pipeline.get(); }

private:
    const IContextVK& GetContextVK() const noexcept;

    vk::UniquePipeline m_vk_unique_pipeline;
};

} // namespace Methane::Graphics
