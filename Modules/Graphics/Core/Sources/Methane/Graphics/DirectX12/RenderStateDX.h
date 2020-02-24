/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Graphics/RenderStateBase.h>

#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class RenderCommandListBase;
class RenderContextDX;
class ProgramDX;

class RenderStateDX final : public RenderStateBase
{
public:
    RenderStateDX(RenderContextBase& context, const Settings& settings);

    // RenderState interface
    void Reset(const Settings& settings) override;
    void SetViewports(const Viewports& viewports) override;
    void SetScissorRects(const ScissorRects& scissor_rects) override;

    // RenderStateBase interface
    void Apply(RenderCommandListBase& command_list, Group::Mask state_groups) override;

    // Object interface
    void SetName(const std::string& name) override;

    void InitializeNativePipelineState();
    wrl::ComPtr<ID3D12PipelineState>& GetNativePipelineState();

private:
    ProgramDX& GetProgramDX();
    RenderContextDX& GetRenderContextDX();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipeline_state_desc = { };
    wrl::ComPtr<ID3D12PipelineState>   m_cp_pipeline_state;
    std::vector<CD3DX12_VIEWPORT>      m_viewports;
    std::vector<CD3DX12_RECT>          m_scissor_rects;
    float                              m_blend_factor[4] = { 0.0, 0.0, 0.0, 0.0 };
};

} // namespace Methane::Graphics
