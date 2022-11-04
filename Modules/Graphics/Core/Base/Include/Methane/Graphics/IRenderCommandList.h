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

FILE: Methane/Graphics/IRenderCommandList.h
Methane render command list interface.

******************************************************************************/

#pragma once

#include "ICommandList.h"
#include "IBuffer.h"
#include "IRenderState.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct IRenderPass;
struct IParallelRenderCommandList;

enum class RenderPrimitive
{
    Point,
    Line,
    LineStrip,
    Triangle,
    TriangleStrip
};

struct IRenderCommandList
    : virtual ICommandList // NOSONAR
{
    using Primitive = RenderPrimitive;

    static constexpr Type type = Type::Render;

    // Create IRenderCommandList instance
    [[nodiscard]] static Ptr<IRenderCommandList> Create(ICommandQueue& command_queue, IRenderPass& render_pass);
    [[nodiscard]] static Ptr<IRenderCommandList> Create(IParallelRenderCommandList& parallel_command_list);
    
    // IRenderCommandList interface
    [[nodiscard]] virtual bool IsValidationEnabled() const noexcept = 0;
    virtual void SetValidationEnabled(bool is_validation_enabled) = 0;
    [[nodiscard]] virtual IRenderPass& GetRenderPass() const = 0;
    virtual void ResetWithState(IRenderState& render_state, IDebugGroup* p_debug_group = nullptr) = 0;
    virtual void ResetWithStateOnce(IRenderState& render_state, IDebugGroup* p_debug_group = nullptr) = 0;
    virtual void SetRenderState(IRenderState& render_state, RenderStateGroups state_groups = RenderStateGroups::All) = 0;
    virtual void SetViewState(IViewState& view_state) = 0;
    virtual bool SetVertexBuffers(IBufferSet& vertex_buffers, bool set_resource_barriers = true) = 0;
    virtual bool SetIndexBuffer(IBuffer& index_buffer, bool set_resource_barriers = true) = 0;
    virtual void DrawIndexed(Primitive primitive, uint32_t index_count = 0, uint32_t start_index = 0, uint32_t start_vertex = 0,
                             uint32_t instance_count = 1, uint32_t start_instance = 0) = 0;
    virtual void Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex = 0,
                      uint32_t instance_count = 1, uint32_t start_instance = 0) = 0;
    
    using ICommandList::Reset;
};

} // namespace Methane::Graphics
