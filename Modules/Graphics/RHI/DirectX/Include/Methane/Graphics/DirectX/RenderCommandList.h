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

FILE: Methane/Graphics/DirectX/RenderCommandList.h
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"

#include <Methane/Graphics/Base/RenderCommandList.h>

namespace Methane::Graphics::DirectX
{

class RenderPass;
class RenderState;

class RenderCommandList final // NOSONAR - inheritance hierarchy greater than 5
    : public CommandList<Base::RenderCommandList>
{
public:
    explicit RenderCommandList(Base::CommandQueue& cmd_queue);
    RenderCommandList(Base::CommandQueue& cmd_queue, Base::RenderPass& render_pass);
    explicit RenderCommandList(Base::ParallelRenderCommandList& parallel_render_command_list);

    // ICommandList interface
    void Commit() override;

    // IRenderCommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    bool SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    void ResetNative(const Ptr<RenderState>& render_state_ptr = nullptr);

private:
    void ResetRenderPass();

    RenderPass& GetDirectPass();
};

} // namespace Methane::Graphics::DirectX
