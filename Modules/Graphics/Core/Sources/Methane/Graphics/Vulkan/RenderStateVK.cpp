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

FILE: Methane/Graphics/Vulkan/RenderStateVK.mm
Vulkan implementation of the render state interface.

******************************************************************************/

#include "RenderStateVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "RenderCommandListVK.h"
#include "ProgramVK.h"
#include "ShaderVK.h"
#include "TypesVK.h"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics
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

Ptr<ViewState> ViewState::Create(const ViewState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ViewStateVK>(state_settings);
}

ViewStateVK::ViewStateVK(const Settings& settings)
    : ViewStateBase(settings)
    , m_vk_viewports(ViewportsToVulkan(settings.viewports))
    , m_vk_scissor_rects(ScissorRectsToVulkan(settings.scissor_rects))
{
    META_FUNCTION_TASK();
}

bool ViewStateVK::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::Reset(settings))
        return false;

    m_vk_viewports     = ViewportsToVulkan(settings.viewports);
    m_vk_scissor_rects = ScissorRectsToVulkan(settings.scissor_rects);
    return true;
}

bool ViewStateVK::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetViewports(viewports))
        return false;

    m_vk_viewports = ViewportsToVulkan(GetSettings().viewports);
    return true;
}

bool ViewStateVK::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!ViewStateBase::SetScissorRects(scissor_rects))
        return false;

    m_vk_scissor_rects = ScissorRectsToVulkan(GetSettings().scissor_rects);
    return true;
}

void ViewStateVK::Apply(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    auto& vulkan_command_list = static_cast<RenderCommandListVK&>(command_list);
    META_UNUSED(vulkan_command_list);
}

Ptr<RenderState> RenderState::Create(const RenderContext& context, const RenderState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderStateVK>(dynamic_cast<const RenderContextBase&>(context), state_settings);
}

RenderStateVK::RenderStateVK(const RenderContextBase& context, const Settings& settings)
    : RenderStateBase(context, settings)
{
    META_FUNCTION_TASK();
    Reset(settings);
}

void RenderStateVK::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(settings.program_ptr, "can not create state with empty program");

    RenderStateBase::Reset(settings);
    ResetNativeState();
}

void RenderStateVK::Apply(RenderCommandListBase& /*command_list*/, Groups /*state_groups*/)
{
    META_FUNCTION_TASK();
}

void RenderStateVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    RenderStateBase::SetName(name);
    
    ResetNativeState();
}

void RenderStateVK::ResetNativeState()
{
    META_FUNCTION_TASK();
}

const IContextVK& RenderStateVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetRenderContext());
}

} // namespace Methane::Graphics
