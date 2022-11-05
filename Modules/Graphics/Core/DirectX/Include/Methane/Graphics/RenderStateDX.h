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

FILE: Methane/Graphics/DirectX12/RenderStateDX.h
DirectX 12 implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/RenderState.h>

#include <wrl.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class Base::RenderCommandList;
class RenderContextDX;
class ProgramDX;

class ViewStateDX final : public Base::ViewState
{
public:
    explicit ViewStateDX(const Settings& settings);

    // IViewState overrides
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // Base::ViewState interface
    void Apply(Base::RenderCommandList& command_list) override;

private:
    std::vector<CD3DX12_VIEWPORT> m_dx_viewports;
    std::vector<CD3DX12_RECT>     m_dx_scissor_rects;
};

class RenderStateDX final : public Base::RenderState
{
public:
    RenderStateDX(const Base::RenderContext& context, const Settings& settings);

    // IRenderState interface
    void Reset(const Settings& settings) override;

    // Base::RenderState interface
    void Apply(Base::RenderCommandList& command_list, Groups state_groups) override;

    // IObject interface
    bool SetName(const std::string& name) override;

    void InitializeNativePipelineState();
    wrl::ComPtr<ID3D12PipelineState>& GetNativePipelineState();

private:
    ProgramDX& GetProgramDX();
    const RenderContextDX& GetRenderContextDX() const noexcept;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipeline_state_desc{ };
    wrl::ComPtr<ID3D12PipelineState>   m_cp_pipeline_state;
    std::array<float, 4>               m_blend_factor{ 0.0, 0.0, 0.0, 0.0 };
};

} // namespace Methane::Graphics
