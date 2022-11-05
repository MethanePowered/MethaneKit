/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

#include <Methane/Graphics/RenderPassDX.h>
#include <Methane/Graphics/RenderContextDX.h>
#include <Methane/Graphics/DescriptorHeapDX.h>
#include <Methane/Graphics/RenderCommandListDX.h>
#include "ResourceDX.hpp"
#include <Methane/Graphics/DeviceDX.h>
#include <Methane/Graphics/TypesDX.h>

#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/Windows/DirectXErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <directx/d3dx12.h>

namespace Methane::Graphics
{

RenderPassDX::AccessDesc::AccessDesc(const Attachment& attachment, const ResourceViewDX& dx_texture_location)
    : descriptor(dx_texture_location.GetNativeCpuDescriptorHandle())
{
    META_FUNCTION_TASK();
    beginning.Type = GetBeginningAccessTypeByLoadAction(attachment.load_action);
    ending.Type    = GetEndingAccessTypeByStoreAction(attachment.store_action);

    if (attachment.store_action == Attachment::StoreAction::Resolve)
    {
        META_FUNCTION_NOT_IMPLEMENTED_DESCR("Resolve parameters initialization is not implemented yet");
    }
}

RenderPassDX::AccessDesc::AccessDesc(const Attachment* attachment_ptr, const ResourceViewDX* dx_texture_location_ptr)
    : descriptor(dx_texture_location_ptr
               ? dx_texture_location_ptr->GetNativeCpuDescriptorHandle()
               : D3D12_CPU_DESCRIPTOR_HANDLE())
{
    META_FUNCTION_TASK();
    if (attachment_ptr)
    {
        beginning.Type = GetBeginningAccessTypeByLoadAction(attachment_ptr->load_action);
        ending.Type    = GetEndingAccessTypeByStoreAction(attachment_ptr->store_action);

        if (attachment_ptr->store_action == Attachment::StoreAction::Resolve)
        {
            META_FUNCTION_NOT_IMPLEMENTED_DESCR("Resolve parameters initialization is not implemented yet");
        }
    }
    else
    {
        beginning.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
        ending.Type    = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
    }
}

RenderPassDX::AccessDesc::AccessDesc(const ColorAttachment& color_attachment, const RenderPassDX& render_pass)
    : AccessDesc(color_attachment, render_pass.GetAttachmentTextureViewDX(color_attachment))
{
    META_FUNCTION_TASK();
}

RenderPassDX::AccessDesc::AccessDesc(const ColorAttachment& color_attachment, const ResourceViewDX& texture_location)
    : AccessDesc(static_cast<const Attachment&>(color_attachment), texture_location)
{
    META_FUNCTION_TASK();

    if (color_attachment.load_action == Attachment::LoadAction::Clear)
    {
        const auto& texture = dynamic_cast<ITexture&>(texture_location.GetResource());
        const DXGI_FORMAT color_format = TypeConverterDX::PixelFormatToDxgi(texture.GetSettings().pixel_format);
        const std::array<float, 4> clear_color_components = color_attachment.clear_color.AsArray();
        beginning.Clear.ClearValue = CD3DX12_CLEAR_VALUE(color_format, clear_color_components.data());
    }
}

RenderPassDX::AccessDesc::AccessDesc(const Opt<DepthAttachment>& depth_attachment_opt, const Opt<StencilAttachment>& stencil_attachment_opt, const RenderPassDX& render_pass)
    : AccessDesc(depth_attachment_opt ? &*depth_attachment_opt : nullptr,
                 depth_attachment_opt ? &render_pass.GetAttachmentTextureViewDX(*depth_attachment_opt) : nullptr)
{
    META_FUNCTION_TASK();
    InitDepthStencilClearValue(depth_attachment_opt, stencil_attachment_opt);
}

RenderPassDX::AccessDesc::AccessDesc(const Opt<StencilAttachment>& stencil_attachment_opt, const Opt<DepthAttachment>& depth_attachment_opt, const RenderPassDX& render_pass)
    : AccessDesc(stencil_attachment_opt ? &*stencil_attachment_opt : nullptr,
                 stencil_attachment_opt ? &render_pass.GetAttachmentTextureViewDX(*stencil_attachment_opt) : nullptr)
{
    META_FUNCTION_TASK();
    InitDepthStencilClearValue(depth_attachment_opt, stencil_attachment_opt);
}

void RenderPassDX::AccessDesc::InitDepthStencilClearValue(const Opt<DepthAttachment>& depth_attachment_opt, const Opt<StencilAttachment>& stencil_attachment_opt)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(depth_attachment_opt, "depth attachment should point to the depth-stencil texture");
    const DXGI_FORMAT depth_format = TypeConverterDX::PixelFormatToDxgi(depth_attachment_opt->format);
    beginning.Clear.ClearValue = CD3DX12_CLEAR_VALUE(depth_format, depth_attachment_opt->clear_value, stencil_attachment_opt ? stencil_attachment_opt->clear_value : 0);
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

RenderPassDX::RTClearInfo::RTClearInfo(const ColorAttachment& color_attach, const RenderPassDX& render_pass)
    : cpu_handle(render_pass.GetAttachmentTextureViewDX(color_attach).GetNativeCpuDescriptorHandle())
    , clear_color(color_attach.clear_color.AsArray())
{
    META_FUNCTION_TASK();
}

RenderPassDX::DSClearInfo::DSClearInfo(const Opt<DepthAttachment>& depth_attach_opt, const Opt<StencilAttachment>& stencil_attach_opt, const RenderPassDX& render_pass)
    : cpu_handle(depth_attach_opt ? render_pass.GetAttachmentTextureViewDX(*depth_attach_opt).GetNativeCpuDescriptorHandle() : D3D12_CPU_DESCRIPTOR_HANDLE())
    , depth_cleared(depth_attach_opt && depth_attach_opt->load_action == IRenderPass::Attachment::LoadAction::Clear)
    , depth_value(depth_attach_opt ? depth_attach_opt->clear_value : 1.F)
    , stencil_cleared(stencil_attach_opt && stencil_attach_opt->load_action == IRenderPass::Attachment::LoadAction::Clear)
    , stencil_value(stencil_attach_opt ? stencil_attach_opt->clear_value : 0)
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

static DescriptorHeapDX::Type GetDescriptorHeapTypeByAccess(IRenderPass::Access access)
{
    META_FUNCTION_TASK();
    switch (access)
    {
    case IRenderPass::Access::ShaderResources: return DescriptorHeapDX::Type::ShaderResources;
    case IRenderPass::Access::Samplers:        return DescriptorHeapDX::Type::Samplers;
    case IRenderPass::Access::RenderTargets:   return DescriptorHeapDX::Type::RenderTargets;
    case IRenderPass::Access::DepthStencil:    return DescriptorHeapDX::Type::DepthStencil;
    default:                                   META_UNEXPECTED_ARG_RETURN(access, DescriptorHeapDX::Type::Undefined);
    }
}

Ptr<IRenderPattern> IRenderPattern::Create(IRenderContext& render_context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPatternBase>(dynamic_cast<RenderContextBase&>(render_context), settings);
}

Ptr<IRenderPass> IRenderPass::Create(Pattern& render_pattern, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderPassDX>(dynamic_cast<RenderPatternBase&>(render_pattern), settings);
}

RenderPassDX::RenderPassDX(RenderPatternBase& render_pattern, const Settings& settings)
    : RenderPassBase(render_pattern, settings, false)
    , m_dx_context(static_cast<const RenderContextDX&>(render_pattern.GetRenderContextBase()))
{
    META_FUNCTION_TASK();
    std::transform(settings.attachments.begin(), settings.attachments.end(), std::back_inserter(m_dx_attachments),
                   [](const ITexture::View& texture_location)
                   { return ResourceViewDX(texture_location, IResource::Usage::RenderTarget); });

    using namespace magic_enum::bitwise_operators;
    if (static_cast<bool>(render_pattern.GetRenderContext().GetSettings().options_mask & IContext::Options::EmulatedRenderPassOnWindows))
    {
        m_is_native_render_pass_available = false;
    }

    // Connect the descriptor heap callback event
    ForEachAccessibleDescriptorHeap([this](DescriptorHeapDX& descriptor_heap)
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
        m_dx_attachments.clear();
        m_native_descriptor_heaps.clear();
        m_native_rt_cpu_handles.clear();
        m_native_ds_cpu_handle = {};
        m_begin_transition_barriers_ptr.reset();
        m_end_transition_barriers_ptr.reset();

        std::transform(settings.attachments.begin(), settings.attachments.end(), std::back_inserter(m_dx_attachments),
                       [](const ITexture::View& texture_location)
                       { return ResourceViewDX(texture_location, IResource::Usage::RenderTarget); });
    }

    if (!m_is_native_render_pass_available.has_value() || m_is_native_render_pass_available.value())
    {
        UpdateNativeRenderPassDesc(settings_changed);
    }
    
    if (!m_is_native_render_pass_available.has_value() || !m_is_native_render_pass_available.value())
    {
        UpdateNativeClearDesc();
    }

    if (settings_changed)
    {
        Data::Emitter<IRenderPassCallback>::Emit(&IRenderPassCallback::OnRenderPassUpdated, *this);
    }

    return settings_changed;
}

void RenderPassDX::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();
    RenderPassBase::ReleaseAttachmentTextures();
    m_dx_attachments.clear();
}

const ResourceViewDX& RenderPassDX::GetAttachmentTextureViewDX(const Attachment& attachment) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(attachment.attachment_index, m_dx_attachments.size(),
                              "attachment index is out of bounds of render pass DX attachments array");
    return m_dx_attachments[attachment.attachment_index];
}


void RenderPassDX::UpdateNativeRenderPassDesc(bool settings_changed)
{
    META_FUNCTION_TASK();
    const Settings& settings = GetSettings();
    const size_t attachment_descriptors_count = m_render_target_descs.size() + (m_depth_stencil_desc ? 1 : 0);
    const bool update_descriptors_only = !settings_changed && attachment_descriptors_count == settings.attachments.size();
    if (!update_descriptors_only)
    {
        m_render_target_descs.clear();
        m_depth_stencil_desc.reset();
    }

    const Pattern::Settings& pattern_settings = GetPatternBase().GetSettings();

    uint32_t color_attachment_index = 0;
    for (const RenderPassBase::ColorAttachment& color_attachment : pattern_settings.color_attachments)
    {
        if (update_descriptors_only)
        {
            m_render_target_descs[color_attachment_index].cpuDescriptor = GetAttachmentTextureViewDX(color_attachment).GetNativeCpuDescriptorHandle();
            color_attachment_index++;
        }
        else
        {
            const AccessDesc render_target_access(color_attachment, *this);
            m_render_target_descs.emplace_back(D3D12_RENDER_PASS_RENDER_TARGET_DESC{
                render_target_access.descriptor, render_target_access.beginning, render_target_access.ending
            });
        }
    }

    if (pattern_settings.depth_attachment)
    {
        if (update_descriptors_only && m_depth_stencil_desc)
        {
            m_depth_stencil_desc->cpuDescriptor = GetAttachmentTextureViewDX(*pattern_settings.depth_attachment).GetNativeCpuDescriptorHandle();
        }
        else
        {
            const AccessDesc depth_access(pattern_settings.depth_attachment, pattern_settings.stencil_attachment, *this);
            const AccessDesc stencil_access(pattern_settings.stencil_attachment, pattern_settings.depth_attachment, *this);

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
    const Pattern::Settings& settings = GetPatternBase().GetSettings();
    for (const RenderPassBase::ColorAttachment& color_attach : settings.color_attachments)
    {
        if (color_attach.load_action != RenderPassBase::Attachment::LoadAction::Clear)
            continue;

        m_rt_clear_infos.emplace_back(color_attach, *this);
    }

    m_ds_clear_info = DSClearInfo(settings.depth_attachment, settings.stencil_attachment, *this);
}

template<typename FuncType>
void RenderPassDX::ForEachAccessibleDescriptorHeap(FuncType do_action) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    const Pattern::Settings& settings = GetPatternBase().GetSettings();

    static constexpr auto s_access_values = magic_enum::enum_values<Access>();
    for (Access access : s_access_values)
    {
        if (!static_cast<bool>(settings.shader_access_mask & access))
            continue;

        const DescriptorHeapDX::Type heap_type = GetDescriptorHeapTypeByAccess(access);
        do_action(m_dx_context.GetDescriptorManagerDX().GetDefaultShaderVisibleDescriptorHeap(heap_type));
    }
}

void RenderPassDX::OnDescriptorHeapAllocated(DescriptorHeapDX&)
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
    SetAttachmentStates(IResource::State::RenderTarget, IResource::State::DepthWrite, m_begin_transition_barriers_ptr, command_list);

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

    if (GetPatternBase().GetSettings().is_final_pass)
    {
        SetAttachmentStates(IResource::State::Present, {}, m_end_transition_barriers_ptr, command_list);
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

    ForEachAccessibleDescriptorHeap([this](DescriptorHeapDX& descriptor_heap)
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

    for (const ColorAttachment& color_attach : GetPatternBase().GetSettings().color_attachments)
    {
        m_native_rt_cpu_handles.push_back(GetAttachmentTextureViewDX(color_attach).GetNativeCpuDescriptorHandle());
    }
    return m_native_rt_cpu_handles;
}

const D3D12_CPU_DESCRIPTOR_HANDLE* RenderPassDX::GetNativeDepthStencilCPUHandle() const
{
    META_FUNCTION_TASK();
    if (m_native_ds_cpu_handle.ptr)
        return &m_native_ds_cpu_handle;

    const Pattern::Settings& settings = GetPatternBase().GetSettings();
    if (!settings.depth_attachment)
        return nullptr;

    m_native_ds_cpu_handle = GetAttachmentTextureViewDX(*settings.depth_attachment).GetNativeCpuDescriptorHandle();
    return &m_native_ds_cpu_handle;
}

} // namespace Methane::Graphics
