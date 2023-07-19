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

FILE: Methane/Graphics/Vulkan/ViewState.cpp
Vulkan implementation of the view state interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ViewState.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/Types.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics::Rhi
{

Ptr<IViewState> Rhi::IViewState::Create(const Rhi::IViewState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::ViewState>(state_settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

[[nodiscard]]
static vk::Viewport ViewportToVulkan(const Viewport& viewport) noexcept
{
    META_FUNCTION_TASK();
    return vk::Viewport(
        static_cast<float>(viewport.origin.GetX()), static_cast<float>(viewport.origin.GetY()),
        static_cast<float>(viewport.size.GetWidth()), static_cast<float>(viewport.size.GetHeight()),
        static_cast<float>(viewport.origin.GetZ()), static_cast<float>(viewport.origin.GetZ() + viewport.size.GetDepth())
    );
}

[[nodiscard]]
static vk::Rect2D ScissorRectToVulkan(const ScissorRect& scissor_rect) noexcept
{
    META_FUNCTION_TASK();
    return vk::Rect2D(
        vk::Offset2D(static_cast<int32_t>(scissor_rect.origin.GetX()), static_cast<int32_t>(scissor_rect.origin.GetY())),
        vk::Extent2D(scissor_rect.size.GetWidth(), scissor_rect.size.GetHeight())
    );
}

[[nodiscard]]
static std::vector<vk::Viewport> ViewportsToVulkan(const Viewports& viewports) noexcept
{
    META_FUNCTION_TASK();
    std::vector<vk::Viewport> vk_viewports;
    std::transform(viewports.begin(), viewports.end(), std::back_inserter(vk_viewports),
                   [](const Viewport& viewport) { return ViewportToVulkan(viewport); });
    return vk_viewports;
}

[[nodiscard]]
static std::vector<vk::Rect2D> ScissorRectsToVulkan(const ScissorRects& scissor_rects) noexcept
{
    META_FUNCTION_TASK();
    std::vector<vk::Rect2D> vk_scissor_rects;
    std::transform(scissor_rects.begin(), scissor_rects.end(), std::back_inserter(vk_scissor_rects),
                   [](const ScissorRect& scissor_rect) { return ScissorRectToVulkan(scissor_rect); });
    return vk_scissor_rects;
}

ViewState::ViewState(const Settings& settings)
    : Base::ViewState(settings)
    , m_vk_viewports(ViewportsToVulkan(settings.viewports))
    , m_vk_scissor_rects(ScissorRectsToVulkan(settings.scissor_rects))
    , m_vk_viewport_state_info({}, m_vk_viewports, m_vk_scissor_rects)
{ }

bool ViewState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::Reset(settings))
        return false;

    m_vk_viewports           = ViewportsToVulkan(settings.viewports);
    m_vk_scissor_rects       = ScissorRectsToVulkan(settings.scissor_rects);
    m_vk_viewport_state_info = vk::PipelineViewportStateCreateInfo({}, m_vk_viewports, m_vk_scissor_rects);
    return true;
}

bool ViewState::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::SetViewports(viewports))
        return false;

    m_vk_viewports = ViewportsToVulkan(GetSettings().viewports);
    m_vk_viewport_state_info.setViewports(m_vk_viewports);
    return true;
}

bool ViewState::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::SetScissorRects(scissor_rects))
        return false;

    m_vk_scissor_rects = ScissorRectsToVulkan(GetSettings().scissor_rects);
    m_vk_viewport_state_info.setScissors(m_vk_scissor_rects);
    return true;
}

void ViewState::Apply(Base::RenderCommandList& command_list)
{
    META_FUNCTION_TASK();
    auto&   vulkan_command_list = static_cast<RenderCommandList&>(command_list);
    if (!vulkan_command_list.IsDynamicStateSupported())
        return;

    const vk::CommandBuffer& vk_command_buffer = vulkan_command_list.GetNativeCommandBufferDefault();
    vk_command_buffer.setViewportWithCountEXT(m_vk_viewports);
    vk_command_buffer.setScissorWithCountEXT(m_vk_scissor_rects);
}

} // namespace Methane::Graphics::Vulkan
