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

FILE: Methane/Graphics/DirectX12/RenderPassDX.cpp
DirectX 12 implementation of the render pass interface.

******************************************************************************/

#include "RenderPassDX.h"
#include "DescriptorHeapDX.h"
#include "RenderCommandListDX.h"
#include "ResourceDX.hpp"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <d3dx12.h>

namespace Methane::Graphics
{

inline D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetTextureCpuDescriptor(const Texture::Location& texture_location)
{
    return texture_location.IsInitialized()
         ? static_cast<ResourceDX<TextureBase>&>(texture_location.GetTexture()).GetNativeCpuDescriptorHandle(Resource::Usage::RenderTarget)
         : D3D12_CPU_DESCRIPTOR_HANDLE();
}

RenderPassDX::AccessDesc::AccessDesc(const Attachment& attachment)
    : descriptor(GetRenderTargetTextureCpuDescriptor(attachment.texture_location))
{
    META_FUNCTION_TASK();

    if (attachment.texture_location.IsInitialized())
    {
        beginning.Type = GetBeginningAccessTypeByLoadAction(attachment.load_action);
        ending.Type    = GetEndingAccessTypeByStoreAction(attachment.store_action);
    }
    else
    {
        beginning.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
        ending.Type    = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
    }

    if (attachment.store_action == Attachment::StoreAction::Resolve)
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("Resolve parameters initialization is not implemented yet");
    }
}

RenderPassDX::AccessDesc::AccessDesc(const ColorAttachment& color_attachment)
    : AccessDesc(static_cast<const Attachment&>(color_attachment))
{
    META_FUNCTION_TASK();

    if (color_attachment.load_action == Attachment::LoadAction::Clear)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(color_attachment.texture_location.IsInitialized(), "can not clear render target attachment without texture");
        const DXGI_FORMAT color_format = TypeConverterDX::PixelFormatToDxgi(color_attachment.texture_location.GetTexture().GetSettings().pixel_format);
        const std::array<float, 4> clear_color_components = color_attachment.clear_color.AsArray();
        beginning.Clear.ClearValue = CD3DX12_CLEAR_VALUE(color_format, clear_color_components.data());
    }
}

RenderPassDX::AccessDesc::AccessDesc(const DepthAttachment& depth_attachment, const StencilAttachment& stencil_attachment)
    : AccessDesc(static_cast<const Attachment&>(depth_attachment))
{
    META_FUNCTION_TASK();
    InitDepthStencilClearValue(depth_attachment, stencil_attachment);
}

RenderPassDX::AccessDesc::AccessDesc(const StencilAttachment& stencil_attachment, const DepthAttachment& depth_attachment)
    : AccessDesc(static_cast<const Attachment&>(stencil_attachment))
{
    META_FUNCTION_TASK();
    InitDepthStencilClearValue(depth_attachment, stencil_attachment);
}

void RenderPassDX::AccessDesc::InitDepthStencilClearValue(const DepthAttachment& depth_attachment, const StencilAttachment& stencil_attachment)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(depth_attachment.texture_location.IsInitialized(), "depth attachment should point to the depth-stencil texture");
    const DXGI_FORMAT depth_format = TypeConverterDX::PixelFormatToDxgi(depth_attachment.texture_location.GetTexture().GetSettings().pixel_format);
    beginning.Clear.ClearValue = CD3DX12_CLEAR_VALUE(depth_format, depth_attachment.clear_value, stencil_attachment.clear_value);
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE RenderPassDX::AccessDesc::GetBeginningAccessTypeByLoadAction(Attachment::LoadAction load_action)
{
    META_FUNCTION_TASK();
    switch (load_action)
    {
    case Attachment::LoadAction::DontCare:  return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
    case Attachment::LoadAction::Load:      return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    case Attachment::LoadAction::Clear:     return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    default:                                META_UNEXPECTED_ARG_RETURN(load_action, D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS);
    }
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE RenderPassDX::AccessDesc::GetEndingAccessTypeByStoreAction(Attachment::StoreAction store_action)
{
    META_FUNCTION_TASK();
    switch (store_action)
    {
    case Attachment::StoreAction::DontCare:  return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
    case Attachment::StoreAction::Store:     return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    case Attachment::StoreAction::Resolve:   return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
    default:                                 META_UNEXPECTED_ARG_RETURN(store_action, D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS);
    }
}

RenderPassDX::RTClearInfo::RTClearInfo(const RenderPass::ColorAttachment& color_attach)
    : cpu_handle(GetRenderTargetTextureCpuDescriptor(color_attach.texture_location))
    , clear_color(color_attach.clear_color.AsArray())
{
    META_FUNCTION_TASK();
}

RenderPassDX::DSClearInfo::DSClearInfo(const RenderPass::DepthAttachment& depth_attach, const RenderPass::StencilAttachment& stencil_attach)
    : cpu_handle(GetRenderTargetTextureCpuDescriptor(depth_attach.texture_location))
    , depth_cleared(depth_attach.texture_location.IsInitialized() && depth_attach.load_action == RenderPass::Attachment::LoadAction::Clear)
    , depth_value(depth_attach.clear_value)
    , stencil_cleared(stencil_attach.texture_location.IsInitialized() && stencil_attach.load_action == RenderPass::Attachment::LoadAction::Clear)
    , stencil_value(stencil_attach.clear_value)
{
    META_FUNCTION_TASK();
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

static DescriptorHeap::Type GetDescriptorHeapTypeByAccess(RenderPass::Access access)
{
    META_FUNCTION_TASK();
    switch (access)
    {
    case RenderPass::Access::ShaderResources: return DescriptorHeap::Type::ShaderResources;
    case RenderPass::Access::Samplers:        return DescriptorHeap::Type::Samplers;
    case RenderPass::Access::RenderTargets:   return DescriptorHeap::Type::RenderTargets;
    case RenderPass::Access::DepthStencil:    return DescriptorHeap::Type::DepthStencil;
    default:                                  META_UNEXPECTED_ARG_RETURN(access, DescriptorHeap::Type::Undefined);
    }
}

Ptr<RenderPass> RenderPass::Create(RenderContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPassDX>(dynamic_cast<RenderContextBase&>(context), settings);
}

RenderPassDX::RenderPassDX(RenderContextBase& context, const Settings& settings)
    : RenderPassBase(context, settings)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    if (magic_enum::flags::enum_contains(context.GetSettings().options_mask & Context::Options::EmulatedRenderPassOnWindows))
    {
        m_is_native_render_pass_available = false;
    }

    // Connect the descriptor heap callback event
    ForEachAccessibleDescriptorHeap([this](DescriptorHeap& descriptor_heap)
    {
        descriptor_heap.Connect(*this);
    });
}

bool RenderPassDX::Update(const Settings& settings)
{
    META_FUNCTION_TASK();
    const bool settings_changed = RenderPassBase::Update(settings);

    if (settings_changed)
    {
        m_native_descriptor_heaps.clear();
        m_native_rt_cpu_handles.clear();
        m_native_ds_cpu_handle = {};
    }

    if (!m_is_native_render_pass_available.has_value() || m_is_native_render_pass_available.value())
    {
        UpdateNativeRenderPassDesc(settings_changed);
    }
    
    if (!m_is_native_render_pass_available.has_value() || !m_is_native_render_pass_available.value())
    {
        UpdateNativeClearDesc();
    }

    return settings_changed;
}

void RenderPassDX::UpdateNativeRenderPassDesc(bool settings_changed)
{
    META_FUNCTION_TASK();

    const Settings& settings = GetSettings();
    const bool update_descriptors_only = !settings_changed && m_render_target_descs.size() == settings.color_attachments.size();
    if (!update_descriptors_only)
    {
        m_render_target_descs.clear();
        m_depth_stencil_desc.reset();
    }

    uint32_t color_attachment_index = 0;
    for (const RenderPassBase::ColorAttachment& color_attachment : settings.color_attachments)
    {
        if (update_descriptors_only)
        {
            m_render_target_descs[color_attachment_index].cpuDescriptor = GetRenderTargetTextureCpuDescriptor(color_attachment.texture_location);
            color_attachment_index++;
        }
        else
        {
            const AccessDesc render_target_access(color_attachment);
            m_render_target_descs.emplace_back(D3D12_RENDER_PASS_RENDER_TARGET_DESC{
                render_target_access.descriptor, render_target_access.beginning, render_target_access.ending
            });
        }
    }

    if (settings.depth_attachment.texture_location.IsInitialized())
    {
        if (update_descriptors_only && m_depth_stencil_desc)
        {
            m_depth_stencil_desc->cpuDescriptor = GetRenderTargetTextureCpuDescriptor(settings.depth_attachment.texture_location);
        }
        else
        {
            const AccessDesc depth_access(settings.depth_attachment, settings.stencil_attachment);
            const AccessDesc stencil_access(settings.stencil_attachment, settings.depth_attachment);

            m_depth_stencil_desc = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
                depth_access.descriptor,
                depth_access.beginning, stencil_access.beginning,
                depth_access.ending,    stencil_access.ending
            };
        }
    }
}

void RenderPassDX::UpdateNativeClearDesc()
{
    META_FUNCTION_TASK();

    m_rt_clear_infos.clear();
    const Settings& settings = GetSettings();
    for (const RenderPassBase::ColorAttachment& color_attach : settings.color_attachments)
    {
        if (color_attach.load_action != RenderPassBase::Attachment::LoadAction::Clear)
            continue;

        META_CHECK_ARG_NOT_NULL_DESCR(color_attach.texture_location.IsInitialized(), "can not clear render target attachment without texture");
        m_rt_clear_infos.emplace_back(color_attach);
    }

    m_ds_clear_info = DSClearInfo(settings.depth_attachment, settings.stencil_attachment);
}

template<typename FuncType>
void RenderPassDX::ForEachAccessibleDescriptorHeap(FuncType do_action) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    const Settings& settings = GetSettings();
    const RenderContextBase& context = GetRenderContext();

    static constexpr auto s_access_values = magic_enum::enum_values<Access>();
    for (Access access : s_access_values)
    {
        if (!magic_enum::flags::enum_contains(settings.shader_access_mask & access))
            continue;

        const DescriptorHeap::Type heap_type = GetDescriptorHeapTypeByAccess(access);
        do_action(context.GetResourceManager().GetDefaultShaderVisibleDescriptorHeap(heap_type));
    }
}

void RenderPassDX::OnDescriptorHeapAllocated(DescriptorHeap&)
{
    META_FUNCTION_TASK();

    // Clear cached native descriptor heaps so that hey will be updated on next request in GetNativeDescriptorHeaps
    m_native_descriptor_heaps.clear();
}

void RenderPassDX::Begin(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    if (!m_is_updated)
    {
        Update(GetSettings());
        m_is_updated = true;
    }

    RenderPassBase::Begin(command_list);

    const auto& command_list_dx = static_cast<const RenderCommandListDX&>(command_list);
    ID3D12GraphicsCommandList& d3d12_command_list = command_list_dx.GetNativeCommandList();

    SetNativeDescriptorHeaps(command_list_dx);

    ID3D12GraphicsCommandList4* p_dx_command_list_4 = command_list_dx.GetNativeCommandList4();
    if (!m_is_native_render_pass_available.has_value() || m_is_native_render_pass_available.value())
    {
        m_is_native_render_pass_available = !!p_dx_command_list_4;
    }

    if (m_is_native_render_pass_available.value())
    {
        // Begin render pass
        p_dx_command_list_4->BeginRenderPass(
            static_cast<UINT>(m_render_target_descs.size()), m_render_target_descs.data(),
            m_depth_stencil_desc ? &*m_depth_stencil_desc : nullptr,
            m_pass_flags
        );
    }
    else
    {
        // Set render targets
        SetNativeRenderTargets(command_list_dx);

        // Clear render targets
        for (const RenderPassDX::RTClearInfo& rt_clear : m_rt_clear_infos)
        {
            d3d12_command_list.ClearRenderTargetView(rt_clear.cpu_handle, rt_clear.clear_color.data(), 0, nullptr);
        }

        // Clear depth-stencil buffer
        if (m_ds_clear_info.depth_cleared || m_ds_clear_info.stencil_cleared)
        {
            d3d12_command_list.ClearDepthStencilView(m_ds_clear_info.cpu_handle, m_ds_clear_info.clear_flags, m_ds_clear_info.depth_value, m_ds_clear_info.stencil_value, 0, nullptr);
        }
    }
}

void RenderPassDX::End(RenderCommandListBase& command_list)
{
    META_FUNCTION_TASK();

    if (m_is_native_render_pass_available.has_value() && m_is_native_render_pass_available.value())
    {
        ID3D12GraphicsCommandList4* p_dx_command_list_4 = static_cast<RenderCommandListDX&>(command_list).GetNativeCommandList4();
        META_CHECK_ARG_NOT_NULL(p_dx_command_list_4);
        p_dx_command_list_4->EndRenderPass();
    }

    RenderPassBase::End(command_list);
}

void RenderPassDX::SetNativeRenderPassUsage(bool use_native_render_pass)
{
    META_FUNCTION_TASK();
    m_is_native_render_pass_available = use_native_render_pass;
}

void RenderPassDX::SetNativeDescriptorHeaps(const RenderCommandListDX& dx_command_list) const
{
    META_FUNCTION_TASK();
    const std::vector<ID3D12DescriptorHeap*>& descriptor_heaps = GetNativeDescriptorHeaps();
    if (descriptor_heaps.empty())
        return;

    dx_command_list.GetNativeCommandList().SetDescriptorHeaps(static_cast<UINT>(descriptor_heaps.size()), descriptor_heaps.data());
}

void RenderPassDX::SetNativeRenderTargets(const RenderCommandListDX& dx_command_list) const
{
    META_FUNCTION_TASK();
    const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rt_cpu_handles = GetNativeRenderTargetCPUHandles();
    const D3D12_CPU_DESCRIPTOR_HANDLE* p_depth_stencil_cpu_handle = GetNativeDepthStencilCPUHandle();
    dx_command_list.GetNativeCommandList().OMSetRenderTargets(static_cast<UINT>(rt_cpu_handles.size()), rt_cpu_handles.data(), FALSE, p_depth_stencil_cpu_handle);
}

const std::vector<ID3D12DescriptorHeap*>& RenderPassDX::GetNativeDescriptorHeaps() const
{
    META_FUNCTION_TASK();
    if (!m_native_descriptor_heaps.empty())
        return m_native_descriptor_heaps;

    ForEachAccessibleDescriptorHeap([this](DescriptorHeap& descriptor_heap)
    {
        m_native_descriptor_heaps.push_back(static_cast<DescriptorHeapDX&>(descriptor_heap).GetNativeDescriptorHeap());
    });

    return m_native_descriptor_heaps;
}

const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RenderPassDX::GetNativeRenderTargetCPUHandles() const
{
    META_FUNCTION_TASK();
    if (!m_native_rt_cpu_handles.empty())
        return m_native_rt_cpu_handles;

    for (const RenderPassBase::ColorAttachment& color_attach : GetSettings().color_attachments)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(color_attach.texture_location.IsInitialized(), "can not use color attachment without texture");
        const auto& rt_texture = static_cast<const ResourceDX<TextureBase>&>(color_attach.texture_location.GetTexture());
        m_native_rt_cpu_handles.push_back(rt_texture.GetNativeCpuDescriptorHandle(ResourceBase::Usage::RenderTarget));
    }

    return m_native_rt_cpu_handles;
}

const D3D12_CPU_DESCRIPTOR_HANDLE* RenderPassDX::GetNativeDepthStencilCPUHandle() const
{
    META_FUNCTION_TASK();
    if (m_native_ds_cpu_handle.ptr)
        return &m_native_ds_cpu_handle;

    const Settings& settings = GetSettings();
    if (!settings.depth_attachment.texture_location.IsInitialized())
        return nullptr;

    const auto& depth_texture = static_cast<const ResourceDX<TextureBase>&>(settings.depth_attachment.texture_location.GetTexture());
    m_native_ds_cpu_handle = depth_texture.GetNativeCpuDescriptorHandle(ResourceBase::Usage::RenderTarget);
    return &m_native_ds_cpu_handle;
}

} // namespace Methane::Graphics
