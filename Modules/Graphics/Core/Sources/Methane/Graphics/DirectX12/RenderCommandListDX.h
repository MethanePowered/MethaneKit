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

FILE: Methane/Graphics/DirectX12/RenderCommandListDX.h
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "RenderPassDX.h"

#include <Methane/Graphics/RenderCommandListBase.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;
class RenderPassDX;

class RenderCommandListDX final : public RenderCommandListBase
{
public:
    RenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass);
    RenderCommandListDX(ParallelRenderCommandListBase& parallel_render_command_list);

    // CommandList interface
    void PushDebugGroup(const std::string& name) override;
    void PopDebugGroup() override;
    void Commit(bool present_drawable) override;

    // CommandListBase interface
    void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) override;
    void Execute(uint32_t frame_index) override;

    void ResetNative(const Ptr<RenderState>& sp_render_state = Ptr<RenderState>());

    // RenderCommandList interface
    void Reset(const Ptr<RenderState>& sp_render_state, const std::string& debug_group) override;
    void SetVertexBuffers(const Refs<Buffer>& vertex_buffers) override;
    void DrawIndexed(Primitive primitive, Buffer& index_buffer,
                     uint32_t index_count, uint32_t start_index, uint32_t start_vertex, 
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    // Object interface
    void SetName(const std::string& name) override;

    const wrl::ComPtr<ID3D12GraphicsCommandList>& GetNativeCommandList() const   { return m_cp_command_list; }
    wrl::ComPtr<ID3D12GraphicsCommandList>&       GetNativeCommandList()         { return m_cp_command_list; }

    const wrl::ComPtr<ID3D12GraphicsCommandList4>& GetNativeCommandList4() const { return m_cp_command_list_4; }
    wrl::ComPtr<ID3D12GraphicsCommandList4>&       GetNativeCommandList4()       { return m_cp_command_list_4; }

protected:
    void Initialize();

    CommandQueueDX& GetCommandQueueDX();
    RenderPassDX&   GetPassDX();

    wrl::ComPtr<ID3D12CommandAllocator>       m_cp_command_allocator;
    wrl::ComPtr<ID3D12GraphicsCommandList>    m_cp_command_list;
    wrl::ComPtr<ID3D12GraphicsCommandList4>   m_cp_command_list_4;    // extended interface for the same command list (may be unavailable on older Windows)
    bool                                      m_is_committed = false;
};

} // namespace Methane::Graphics
