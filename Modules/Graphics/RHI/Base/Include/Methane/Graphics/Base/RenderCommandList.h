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

FILE: Methane/Graphics/Base/RenderCommandList.h
Base implementation of the render command list interface.

******************************************************************************/

#pragma once

#include "CommandList.h"

#include <Methane/Graphics/RHI/IRenderCommandList.h>

#include <Methane/Data/EnumMask.hpp>

#include <optional>

namespace Methane::Graphics::Rhi
{

struct IRenderState;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

class ParallelRenderCommandList;
class ViewState;
class RenderState;
class RenderPass;
class BufferSet;
class Buffer;
class Texture;

struct RenderDrawingState
{
    enum class Change : uint32_t
    {
        PrimitiveType,
        ViewState
    };

    using ChangeMask = Data::EnumMask<Change>;

    Ptrs<Texture>             render_pass_attachment_ptrs;
    Ptr<RenderState>          render_state_ptr;
    Ptr<BufferSet>            vertex_buffer_set_ptr;
    Ptr<Buffer>               index_buffer_ptr;
    Opt<Rhi::RenderPrimitive> primitive_type_opt;
    ViewState*                view_state_ptr      = nullptr;
    Rhi::RenderStateGroupMask render_state_groups;
    ChangeMask                changes;
};

class RenderCommandList
    : public Rhi::IRenderCommandList
    , public CommandList
{
public:
    using DrawingState = RenderDrawingState;

    static Ptr<IRenderCommandList> CreateForSynchronization(Rhi::ICommandQueue& cmd_queue);

    explicit RenderCommandList(CommandQueue& command_queue);
    RenderCommandList(CommandQueue& command_queue, RenderPass& render_pass);
    explicit RenderCommandList(ParallelRenderCommandList& parallel_render_command_list);
    
    using CommandList::Reset;

    // IRenderCommandList interface
    bool IsValidationEnabled() const noexcept final             { return m_is_validation_enabled; }
    void SetValidationEnabled(bool is_validation_enabled) final { m_is_validation_enabled = is_validation_enabled; }
    Rhi::IRenderPass& GetRenderPass() const final;
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) override;
    void ResetWithStateOnce(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr = nullptr) final;
    void SetRenderState(Rhi::IRenderState& render_state, Rhi::RenderStateGroupMask state_groups = Rhi::RenderStateGroupMask(~0U)) override;
    void SetViewState(Rhi::IViewState& view_state) override;
    bool SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers) override;
    bool SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers) override;
    void DrawIndexed(Primitive primitive_type, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                     uint32_t instance_count, uint32_t start_instance) override;
    void Draw(Primitive primitive_type, uint32_t vertex_count, uint32_t start_vertex,
              uint32_t instance_count, uint32_t start_instance) override;

    RenderPass&         GetPass();
    RenderPass*         GetPassPtr() const noexcept      { return m_render_pass_ptr.get(); }
    bool                HasPass() const noexcept         { return !!m_render_pass_ptr; }
    const DrawingState& GetDrawingState() const noexcept { return m_drawing_state; }

protected:
    // CommandList overrides
    void ResetCommandState() override;

    DrawingState& GetDrawingState() noexcept  { return m_drawing_state; }
    bool          IsParallel() const noexcept { return m_is_parallel; }

    inline void UpdateDrawingState(Primitive primitive_type);
    inline void ValidateDrawVertexBuffers(uint32_t draw_start_vertex, uint32_t draw_vertex_count = 0) const;

private:
    const bool            m_is_parallel = false;
    const Ptr<RenderPass> m_render_pass_ptr;
    DrawingState          m_drawing_state;
    bool                  m_is_validation_enabled = true;
};

} // namespace Methane::Graphics::Base
