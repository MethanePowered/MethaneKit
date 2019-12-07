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

FILE: Methane/Graphics/DirectX12/RenderPassDX.cpp
DirectX 12 implementation of the render pass interface.

******************************************************************************/

#include "RenderPassDX.h"
#include "DescriptorHeapDX.h"
#include "RenderCommandListDX.h"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/TextureBase.h>

#include <cassert>

namespace Methane::Graphics
{

static DescriptorHeap::Type GetDescriptorHeapTypeByAccess(RenderPass::Access::Value access) noexcept
{
    ITT_FUNCTION_TASK();
    
    switch (access)
    {
    case RenderPass::Access::ShaderResources: return DescriptorHeap::Type::ShaderResources;
    case RenderPass::Access::Samplers:        return DescriptorHeap::Type::Samplers;
    case RenderPass::Access::RenderTargets:   return DescriptorHeap::Type::RenderTargets;
    case RenderPass::Access::DepthStencil:    return DescriptorHeap::Type::DepthStencil;
    }
    assert(0);
    return DescriptorHeap::Type::Undefined;
}

RenderPassDX::RTClearInfo::RTClearInfo(const RenderPass::ColorAttachment& color_attach)
    : cpu_handle(!color_attach.wp_texture.expired() ? static_cast<TextureBase&>(*color_attach.wp_texture.lock()).GetNativeCPUDescriptorHandle(Resource::Usage::RenderTarget)
                                                    : D3D12_CPU_DESCRIPTOR_HANDLE())
    , clear_color{ color_attach.clear_color.r(), color_attach.clear_color.g(), color_attach.clear_color.b(), color_attach.clear_color.a() }
{
    ITT_FUNCTION_TASK();
}

RenderPassDX::DSClearInfo::DSClearInfo(const RenderPass::DepthAttachment& depth_attach, const RenderPass::StencilAttachment& stencil_attach)
    : cpu_handle(!depth_attach.wp_texture.expired() ? static_cast<TextureBase&>(*depth_attach.wp_texture.lock()).GetNativeCPUDescriptorHandle(ResourceBase::Usage::RenderTarget)
                                                    : !stencil_attach.wp_texture.expired() ? static_cast<TextureBase&>(*stencil_attach.wp_texture.lock()).GetNativeCPUDescriptorHandle(ResourceBase::Usage::RenderTarget)
                                                                                           : D3D12_CPU_DESCRIPTOR_HANDLE())
    , depth_cleared(!depth_attach.wp_texture.expired() && depth_attach.load_action == RenderPass::Attachment::LoadAction::Clear)
    , depth_value(depth_attach.clear_value)
    , stencil_cleared(!stencil_attach.wp_texture.expired() && stencil_attach.load_action == RenderPass::Attachment::LoadAction::Clear)
    , stencil_value(stencil_attach.clear_value)
{
    ITT_FUNCTION_TASK();

    if (depth_cleared)
    {
        clear_flags = stencil_cleared ? D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL
                                      : D3D12_CLEAR_FLAG_DEPTH;
    }
    else if (stencil_cleared)
    {
        clear_flags = D3D12_CLEAR_FLAG_STENCIL;
    }
}

RenderPass::Ptr RenderPass::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderPassDX>(static_cast<ContextBase&>(context), settings);
}

RenderPassDX::RenderPassDX(ContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    ITT_FUNCTION_TASK();
}

void RenderPassDX::Update(const Settings& settings)
{
    ITT_FUNCTION_TASK();

    RenderPassBase::Update(settings);

    m_rt_clear_infos.clear();
    for (const RenderPassBase::ColorAttachment& color_attach : m_settings.color_attachments)
    {
        if (color_attach.load_action != RenderPassBase::Attachment::LoadAction::Clear)
            continue;

        if (color_attach.wp_texture.expired())
        {
            throw std::invalid_argument("Can not clear render target attachement without texture.");
        }
        m_rt_clear_infos.emplace_back(color_attach);
    }

    m_ds_clear_info = DSClearInfo(m_settings.depth_attachment, m_settings.stencil_attachment);
}

void RenderPassDX::Apply(RenderCommandListBase& command_list)
{
    ITT_FUNCTION_TASK();

    if (!m_is_updated)
    {
        Update(m_settings);
        m_is_updated = true;
    }

    RenderPassBase::Apply(command_list);

    wrl::ComPtr<ID3D12GraphicsCommandList>& cp_dx_command_list = static_cast<RenderCommandListDX&>(command_list).GetNativeCommandList();

    // Set descriptor heaps
    const std::vector<ID3D12DescriptorHeap*> descritor_heaps = GetNativeDescriptorHeaps();
    if (!descritor_heaps.empty())
    {
        cp_dx_command_list->SetDescriptorHeaps(static_cast<UINT>(descritor_heaps.size()), descritor_heaps.data());
    }

    // Set render targets
    const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rt_cpu_handles = GetNativeRenderTargetCPUHandles();
    const D3D12_CPU_DESCRIPTOR_HANDLE* p_depth_stencil_cpu_handle = GetNativeDepthStencilCPUHandle();
    cp_dx_command_list->OMSetRenderTargets(static_cast<UINT>(rt_cpu_handles.size()), rt_cpu_handles.data(), FALSE, p_depth_stencil_cpu_handle);

    // Clear render targets
    for (const RenderPassDX::RTClearInfo& rt_clear : m_rt_clear_infos)
    {
        cp_dx_command_list->ClearRenderTargetView(rt_clear.cpu_handle, rt_clear.clear_color, 0, nullptr);
    }

    // Clear depth-stencil buffer
    if (m_ds_clear_info.depth_cleared || m_ds_clear_info.stencil_cleared)
    {
        cp_dx_command_list->ClearDepthStencilView(m_ds_clear_info.cpu_handle, m_ds_clear_info.clear_flags, m_ds_clear_info.depth_value, m_ds_clear_info.stencil_value, 0, nullptr);
    }
}

std::vector<ID3D12DescriptorHeap*> RenderPassDX::GetNativeDescriptorHeaps() const
{
    ITT_FUNCTION_TASK();
    std::vector<ID3D12DescriptorHeap*> descritor_heaps;
    for (Access::Value access : Access::values)
    {
        if (!(m_settings.shader_access_mask & access))
            continue;

        DescriptorHeap::Type heap_type = GetDescriptorHeapTypeByAccess(access);
        DescriptorHeapDX& descriptor_heap_dx = static_cast<DescriptorHeapDX&>(m_context.GetResourceManager().GetDefaultShaderVisibleDescriptorHeap(heap_type));
        descritor_heaps.push_back(descriptor_heap_dx.GetNativeDescriptorHeap());
    }
    return descritor_heaps;
}

std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RenderPassDX::GetNativeRenderTargetCPUHandles() const
{
    ITT_FUNCTION_TASK();
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rt_cpu_handles;
    for (const RenderPassBase::ColorAttachment& color_attach : m_settings.color_attachments)
    {
        if (color_attach.wp_texture.expired())
        {
            throw std::invalid_argument("Can not use color attachment without texture.");
        }
        rt_cpu_handles.push_back(static_cast<TextureBase&>(*color_attach.wp_texture.lock()).GetNativeCPUDescriptorHandle(ResourceBase::Usage::RenderTarget));
    }
    return rt_cpu_handles;
}

const D3D12_CPU_DESCRIPTOR_HANDLE* RenderPassDX::GetNativeDepthStencilCPUHandle()
{
    ITT_FUNCTION_TASK();
    if (!m_settings.depth_attachment.wp_texture.expired())
    {
        m_depth_stencil_cpu_handle = static_cast<TextureBase&>(*m_settings.depth_attachment.wp_texture.lock()).GetNativeCPUDescriptorHandle(ResourceBase::Usage::RenderTarget);
        return &m_depth_stencil_cpu_handle;
    }
    return nullptr;
}

} // namespace Methane::Graphics
