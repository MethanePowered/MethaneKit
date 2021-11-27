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

FILE: Methane/Graphics/DirectX12/RenderPassDX.h
DirectX 12 implementation of the render pass interface.

******************************************************************************/

#pragma once

#include "DescriptorHeapDX.h"

#include <Methane/Graphics/RenderPassBase.h>
#include <Methane/Data/Receiver.hpp>

#include <d3d12.h>

#include <vector>
#include <optional>

namespace Methane::Graphics
{

class RenderCommandListDX;

class RenderPassDX final
    : public RenderPassBase
    , private Data::Receiver<IDescriptorHeapCallback> //NOSONAR
{
public:
    RenderPassDX(RenderPatternBase& render_pattern, const Settings& settings);

    // RenderPass interface
    bool Update(const Settings& settings) override;

    // RenderPassBase interface
    void Begin(RenderCommandListBase& command_list) override;
    void End(RenderCommandListBase& command_list) override;

    // Allows to disable native D3D12 render-pass feature usage,
    // but enabling does not guarantee that it will be used (it depends on OS version and API availability)
    void SetNativeRenderPassUsage(bool use_native_render_pass);
    void SetNativeDescriptorHeaps(const RenderCommandListDX& dx_command_list) const;
    void SetNativeRenderTargets(const RenderCommandListDX& dx_command_list) const;

    const std::vector<ID3D12DescriptorHeap*>&       GetNativeDescriptorHeaps() const;
    const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& GetNativeRenderTargetCPUHandles() const;
    const D3D12_CPU_DESCRIPTOR_HANDLE*              GetNativeDepthStencilCPUHandle() const;

private:
    struct AccessDesc
    {
        D3D12_CPU_DESCRIPTOR_HANDLE        descriptor { };
        D3D12_RENDER_PASS_BEGINNING_ACCESS beginning  { };
        D3D12_RENDER_PASS_ENDING_ACCESS    ending     { };

        explicit AccessDesc(const Attachment& attachment, const Texture::Location& texture_location);
        explicit AccessDesc(const Attachment* attachment_ptr, const Texture::Location* texture_location_ptr);
        explicit AccessDesc(const ColorAttachment& color_attachment, const RenderPassBase& render_pass);
        explicit AccessDesc(const ColorAttachment& color_attachment, const Texture::Location& texture_location);
        AccessDesc(const Opt<DepthAttachment>& depth_attachment_opt, const Opt<StencilAttachment>& stencil_attachment_opt, const RenderPassBase& render_pass);
        AccessDesc(const Opt<StencilAttachment>& stencil_attachment_opt, const Opt<DepthAttachment>& depth_attachment_opt, const RenderPassBase& render_pass);

        void InitDepthStencilClearValue(const Opt<DepthAttachment>& depth_attachment_opt, const Opt<StencilAttachment>& stencil_attachment_opt);

        static D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE GetBeginningAccessTypeByLoadAction(Attachment::LoadAction load_action);
        static D3D12_RENDER_PASS_ENDING_ACCESS_TYPE    GetEndingAccessTypeByStoreAction(Attachment::StoreAction store_action);
    };

    struct RTClearInfo
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle  { };
        std::array<float, 4>        clear_color { };

        RTClearInfo(const ColorAttachment& color_attach, const RenderPassBase& render_pass);
    };

    struct DSClearInfo
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle      { };
        D3D12_CLEAR_FLAGS           clear_flags     { };
        bool                        depth_cleared   = false;
        FLOAT                       depth_value     = 1.F;
        bool                        stencil_cleared = false;
        UINT8                       stencil_value   = 0;

        DSClearInfo() = default;
        DSClearInfo(const Opt<DepthAttachment>& depth_attach_opt, const Opt<StencilAttachment>& stencil_attach_opt, const RenderPassBase& render_pass);
    };

    void UpdateNativeRenderPassDesc(bool settings_changed);
    void UpdateNativeClearDesc();

    template<typename FuncType> // function void(DescriptorHeapDX& descriptor_heap)
    void ForEachAccessibleDescriptorHeap(FuncType do_action) const;

    // IDescriptorHeapCallback implementation
    void OnDescriptorHeapAllocated(DescriptorHeapDX& descriptor_heap) override;

    // D3D12 Render-Pass description
    std::optional<bool>                                 m_is_native_render_pass_available;
    std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC>   m_render_target_descs;
    std::optional<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> m_depth_stencil_desc;
    D3D12_RENDER_PASS_FLAGS                             m_pass_flags = D3D12_RENDER_PASS_FLAG_NONE;

    // Fallback to input assembler setup
    std::vector<RTClearInfo>                            m_rt_clear_infos;
    DSClearInfo                                         m_ds_clear_info;
    bool                                                m_is_updated = false;

    // Cache of native type vectors to minimize memory allocation during rendering
    mutable std::vector<ID3D12DescriptorHeap*>          m_native_descriptor_heaps;
    mutable std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>    m_native_rt_cpu_handles;
    mutable D3D12_CPU_DESCRIPTOR_HANDLE                 m_native_ds_cpu_handle{ };
};

} // namespace Methane::Graphics
