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

FILE: Methane/Graphics/Metal/ViewState.mm
Metal implementation of the view state interface.

******************************************************************************/

#include <Methane/Graphics/Metal/ViewState.hh>
#include <Methane/Graphics/Metal/RenderCommandList.hh>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IViewState> IViewState::Create(const ViewSettings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::ViewState>(state_settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Metal
{

static std::vector<MTLViewport> ConvertViewportsToMetal(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    std::vector<MTLViewport> mtl_viewports;
    mtl_viewports.reserve(viewports.size());

    for(const Viewport& viewport : viewports)
    {
        MTLViewport mtl_viewport{ };
        mtl_viewport.originX = viewport.origin.GetX();
        mtl_viewport.originY = viewport.origin.GetY();
        mtl_viewport.width   = viewport.size.GetWidth();
        mtl_viewport.height  = viewport.size.GetHeight();
        mtl_viewport.znear   = viewport.origin.GetZ();
        mtl_viewport.zfar    = viewport.origin.GetZ() + viewport.size.GetDepth();
        mtl_viewports.emplace_back(std::move(mtl_viewport));
    }

    return mtl_viewports;
}

static std::vector<MTLScissorRect> ConvertScissorRectsToMetal(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    std::vector<MTLScissorRect> mtl_scissor_rects;
    mtl_scissor_rects.reserve(scissor_rects.size());

    for(const ScissorRect& scissor_rect : scissor_rects)
    {
        MTLScissorRect mtl_scissor_rect{};
        mtl_scissor_rect.x      = static_cast<NSUInteger>(scissor_rect.origin.GetX());
        mtl_scissor_rect.y      = static_cast<NSUInteger>(scissor_rect.origin.GetY());
        mtl_scissor_rect.width  = static_cast<NSUInteger>(scissor_rect.size.GetWidth());
        mtl_scissor_rect.height = static_cast<NSUInteger>(scissor_rect.size.GetHeight());
        mtl_scissor_rects.emplace_back(std::move(mtl_scissor_rect));
    }

    return mtl_scissor_rects;
}

ViewState::ViewState(const Settings& settings)
    : Base::ViewState(settings)
    , m_mtl_viewports(ConvertViewportsToMetal(settings.viewports))
    , m_mtl_scissor_rects(ConvertScissorRectsToMetal(settings.scissor_rects))
{
    META_FUNCTION_TASK();
}

bool ViewState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::Reset(settings))
        return false;

    m_mtl_viewports     = ConvertViewportsToMetal(settings.viewports);
    m_mtl_scissor_rects = ConvertScissorRectsToMetal(settings.scissor_rects);
    return true;
}

bool ViewState::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::SetViewports(viewports))
        return false;

    m_mtl_viewports = ConvertViewportsToMetal(viewports);
    return true;
}

bool ViewState::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::SetScissorRects(scissor_rects))
        return false;

    m_mtl_scissor_rects = ConvertScissorRectsToMetal(scissor_rects);
    return true;
}

void ViewState::Apply(Base::RenderCommandList& command_list)
{
    META_FUNCTION_TASK();

    RenderCommandList& metal_command_list = static_cast<RenderCommandList&>(command_list);
    const id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeCommandEncoder();

    [mtl_cmd_encoder setViewports: m_mtl_viewports.data() count:static_cast<uint32_t>(m_mtl_viewports.size())];
    [mtl_cmd_encoder setScissorRects: m_mtl_scissor_rects.data() count:static_cast<uint32_t>(m_mtl_scissor_rects.size())];
}

} // namespace Methane::Graphics::Metal
