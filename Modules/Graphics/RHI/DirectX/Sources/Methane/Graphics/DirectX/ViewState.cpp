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

FILE: Methane/Graphics/DirectX/ViewState.cpp
DirectX 12 implementation of the view state interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/ViewState.h>
#include <Methane/Graphics/DirectX/Types.h>
#include <Methane/Graphics/DirectX/RenderCommandList.h>

#include <Methane/Graphics/DirectX/ErrorHandling.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <directx/d3dx12_core.h>

#include <algorithm>

namespace Methane::Graphics::Rhi
{

Ptr<IViewState> Rhi::IViewState::Create(const Rhi::IViewState::Settings& state_settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::ViewState>(state_settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

[[nodiscard]]
static D3D12_VIEWPORT ViewportToD3D(const Viewport& viewport) noexcept
{
    META_FUNCTION_TASK();
    return CD3DX12_VIEWPORT(static_cast<float>(viewport.origin.GetX()),   static_cast<float>(viewport.origin.GetY()),
                            static_cast<float>(viewport.size.GetWidth()), static_cast<float>(viewport.size.GetHeight()),
                            static_cast<float>(viewport.origin.GetZ()),   static_cast<float>(viewport.origin.GetZ() + viewport.size.GetDepth()));
}

[[nodiscard]]
static D3D12_RECT ScissorRectToD3D(const ScissorRect& scissor_rect) noexcept
{
    META_FUNCTION_TASK();
    return CD3DX12_RECT(static_cast<LONG>(scissor_rect.origin.GetX()), static_cast<LONG>(scissor_rect.origin.GetY()),
                        static_cast<LONG>(scissor_rect.origin.GetX() + scissor_rect.size.GetWidth()),
                        static_cast<LONG>(scissor_rect.origin.GetY() + scissor_rect.size.GetHeight()));
}

[[nodiscard]]
static std::vector<D3D12_VIEWPORT> ViewportsToD3D(const Viewports& viewports) noexcept
{
    META_FUNCTION_TASK();
    std::vector<D3D12_VIEWPORT> d3d_viewports;
    std::ranges::transform(viewports, std::back_inserter(d3d_viewports),
                   [](const Viewport& viewport) { return ViewportToD3D(viewport); });
    return d3d_viewports;
}

[[nodiscard]]
static std::vector<D3D12_RECT> ScissorRectsToD3D(const ScissorRects& scissor_rects) noexcept
{
    META_FUNCTION_TASK();
    std::vector<D3D12_RECT> d3d_scissor_rects;
    std::ranges::transform(scissor_rects, std::back_inserter(d3d_scissor_rects),
                   [](const ScissorRect& scissor_rect) { return ScissorRectToD3D(scissor_rect); });
    return d3d_scissor_rects;
}

ViewState::ViewState(const Settings& settings)
    : Base::ViewState(settings)
    , m_dx_viewports(ViewportsToD3D(settings.viewports))
    , m_dx_scissor_rects(ScissorRectsToD3D(settings.scissor_rects))
{ }

bool ViewState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::Reset(settings))
        return false;

    m_dx_viewports     = ViewportsToD3D(settings.viewports);
    m_dx_scissor_rects = ScissorRectsToD3D(settings.scissor_rects);
    return true;
}

bool ViewState::SetViewports(const Viewports& viewports)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::SetViewports(viewports))
        return false;

    m_dx_viewports = ViewportsToD3D(viewports);
    return true;
}

bool ViewState::SetScissorRects(const ScissorRects& scissor_rects)
{
    META_FUNCTION_TASK();
    if (!Base::ViewState::SetScissorRects(scissor_rects))
        return false;

    m_dx_scissor_rects = ScissorRectsToD3D(scissor_rects);
    return true;
}

void ViewState::Apply(Base::RenderCommandList& command_list)
{
    META_FUNCTION_TASK();
    const auto& dx_render_command_list = static_cast<const RenderCommandList&>(command_list);
    ID3D12GraphicsCommandList& d3d12_command_list = dx_render_command_list.GetNativeCommandList();

    d3d12_command_list.RSSetViewports(static_cast<UINT>(m_dx_viewports.size()), m_dx_viewports.data());
    d3d12_command_list.RSSetScissorRects(static_cast<UINT>(m_dx_scissor_rects.size()), m_dx_scissor_rects.data());
}

} // namespace Methane::Graphics::DirectX
