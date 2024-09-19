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

FILE: Methane/Graphics/DirectX/RenderState.h
DirectX 12 implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/RenderState.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class RenderContext;
class Program;

class RenderState final
    : public Base::RenderState
{
public:
    RenderState(const Base::RenderContext& context, const Settings& settings);

    // IRenderState interface
    void Reset(const Settings& settings) override;

    // Base::RenderState interface
    void Apply(Base::RenderCommandList& command_list, Groups state_groups) override;

    // IObject interface
    bool SetName(std::string_view name) override;

    void InitializeNativePipelineState();
    wrl::ComPtr<ID3D12PipelineState>& GetNativePipelineState();

private:
    Program& GetDirectProgram();
    const RenderContext& GetDirectRenderContext() const noexcept;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipeline_state_desc{ };
    wrl::ComPtr<ID3D12PipelineState>   m_pipeline_state_cptr;
    std::array<float, 4>               m_blend_factor{ 0.0, 0.0, 0.0, 0.0 };
};

} // namespace Methane::Graphics::DirectX
