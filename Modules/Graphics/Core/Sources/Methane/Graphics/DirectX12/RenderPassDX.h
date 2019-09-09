/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX12/RenderPassDX.h
DirectX 12 implementation of the render pass interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderPassBase.h>

#include <d3d12.h>

namespace Methane::Graphics
{

class RenderCommandListDX;

class RenderPassDX : public RenderPassBase
{
public:
    struct RTClearInfo
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle      = {};
        float                       clear_color[4]  = {};

        RTClearInfo(const RenderPassBase::ColorAttachment& color_attach);
    };

    struct DSClearInfo
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle      = {};
        D3D12_CLEAR_FLAGS           clear_flags     = {};
        bool                        depth_cleared   = false;
        FLOAT                       depth_value     = 1.f;
        bool                        stencil_cleared = false;
        UINT8                       stencil_value   = 0;

        DSClearInfo() {}
        DSClearInfo(const RenderPassBase::DepthAttachment& depth_attach, const RenderPassBase::StencilAttachment& stencil_attach);
    };

    RenderPassDX(ContextBase& context, const Settings& settings);

    // RenderPass interface
    void Update(const Settings& settings) override;

    // RenderPassBase interface
    void Apply(RenderCommandListBase& command_list) override;

    std::vector<ID3D12DescriptorHeap*>       GetNativeDescriptorHeaps() const;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetNativeRenderTargetCPUHandles() const;
    const D3D12_CPU_DESCRIPTOR_HANDLE*       GetNativeDepthStencilCPUHandle();
    const std::vector<RTClearInfo>&          GetNativeRenderTargetsClearInfo() const    { return m_rt_clear_infos; }
    const DSClearInfo&                       GetNativeDepthStencilClearInfo() const     { return m_ds_clear_info; }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE             m_depth_stencil_cpu_handle = {};
    std::vector<RenderPassDX::RTClearInfo>  m_rt_clear_infos;
    DSClearInfo                             m_ds_clear_info;
    bool                                    m_is_updated = false;
};

} // namespace Methane::Graphics
