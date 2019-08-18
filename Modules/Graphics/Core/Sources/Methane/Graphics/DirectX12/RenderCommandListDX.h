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

FILE: Methane/Graphics/DirectX12/RenderCommandListDX.h
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderCommandListBase.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane
{
namespace Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;
class RenderPassDX;
class DeviceDX;

class RenderCommandListDX final : public RenderCommandListBase
{
public:
    RenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass);

    // CommandList interface
    void PushDebugGroup(const std::string& name) override;
    void PopDebugGroup() override;
    void Commit(bool present_drawable) override;

    // CommandListBase interface
    void SetResourceBarriers(const ResourceBase::Barriers& resource_barriers) override;
    void Execute(uint32_t frame_index) override;

    // RenderCommandList interface
    void Reset(RenderState& render_state, const std::string& debug_group) override;
    void SetVertexBuffers(const Buffer::Refs& vertex_buffers) override;
    void DrawIndexed(Primitive primitive, const Buffer& index_buffer, uint32_t instance_count) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t instance_count) override;

    // Object interface
    void SetName(const std::string& name) override;

    // Context::ICallback
    void OnContextReset(Device& device) override;

    wrl::ComPtr<ID3D12GraphicsCommandList>& GetNativeCommandList()      { return m_cp_command_list; }

protected:
    void Initialize(const DeviceDX& device);
    CommandQueueDX& GetCommandQueueDX();
    RenderPassDX&   GetPassDX();

    wrl::ComPtr<ID3D12CommandAllocator>     m_cp_command_allocator;
    wrl::ComPtr<ID3D12GraphicsCommandList>  m_cp_command_list;
    Resource::Refs                          m_present_resources;
    bool                                    m_is_committed = false;
};

} // namespace Graphics
} // namespace Methane
