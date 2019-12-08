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

#include <vector>
#include <optional>

#ifndef D3D12_RENDER_PASS_ENABLED
#define D3D12_RENDER_PASS_ENABLED 1
#endif

namespace Methane::Graphics
{

class RenderCommandListDX;

class RenderPassDX : public RenderPassBase
{
public:
    RenderPassDX(ContextBase& context, const Settings& settings);

    // RenderPass interface
    void Update(const Settings& settings) override;

    // RenderPassBase interface
    void Begin(RenderCommandListBase& command_list) override;
    void End(RenderCommandListBase& command_list) override;

    std::vector<ID3D12DescriptorHeap*>       GetNativeDescriptorHeaps() const;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetNativeRenderTargetCPUHandles() const;
    const D3D12_CPU_DESCRIPTOR_HANDLE*       GetNativeDepthStencilCPUHandle();

private:
#if D3D12_RENDER_PASS_ENABLED

    struct AccessDesc
    {
        D3D12_CPU_DESCRIPTOR_HANDLE        descriptor = { };
        D3D12_RENDER_PASS_BEGINNING_ACCESS beginning  = { };
        D3D12_RENDER_PASS_ENDING_ACCESS    ending     = { };

        AccessDesc(const Attachment& attachment);
        AccessDesc(const ColorAttachment& color_attachment);
        AccessDesc(const DepthAttachment& depth_attachment, const StencilAttachment& stencil_attachment);
        AccessDesc(const StencilAttachment& stencil_attachment, const DepthAttachment& depth_attachment);

        void InitDepthStencilClearValue(const DepthAttachment& depth_attachment, const StencilAttachment& stencil_attachment);

        static D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE GetBeginningAccessTypeByLoadAction(Attachment::LoadAction load_action) noexcept;
        static D3D12_RENDER_PASS_ENDING_ACCESS_TYPE    GetEndingAccessTypeByStoreAction(Attachment::StoreAction store_action) noexcept;
    };

    std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC>   m_render_target_descs;
    std::optional<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> m_depth_stencil_desc;
    D3D12_RENDER_PASS_FLAGS                             m_pass_flags = D3D12_RENDER_PASS_FLAG_NONE;

#else

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

    std::vector<RTClearInfo>                            m_rt_clear_infos;
    DSClearInfo                                         m_ds_clear_info;

#endif

    D3D12_CPU_DESCRIPTOR_HANDLE                         m_depth_stencil_cpu_handle = {};
    bool                                                m_is_updated = false;
};

} // namespace Methane::Graphics
