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

FILE: Methane/Graphics/Metal/RenderCommandListMT.mm
Metal implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListMT.hh"
#include "CommandListMT.hh"
#include "ParallelRenderCommandListMT.hh"
#include "RenderPassMT.hh"
#include "RenderContextMT.hh"
#include "BufferMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

static MTLPrimitiveType PrimitiveTypeToMetal(RenderCommandList::Primitive primitive) noexcept
{
    META_FUNCTION_TASK();

    switch (primitive)
    {
        case RenderCommandList::Primitive::Point:          return MTLPrimitiveTypePoint;
        case RenderCommandList::Primitive::Line:           return MTLPrimitiveTypeLine;
        case RenderCommandList::Primitive::LineStrip:      return MTLPrimitiveTypeLineStrip;
        case RenderCommandList::Primitive::Triangle:       return MTLPrimitiveTypeTriangle;
        case RenderCommandList::Primitive::TriangleStrip:  return MTLPrimitiveTypeTriangleStrip;
    }
    return MTLPrimitiveTypeTriangleStrip;
}

Ptr<RenderCommandList> RenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListMT>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

Ptr<RenderCommandList> RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListMT>(static_cast<ParallelRenderCommandListBase&>(parallel_render_command_list));
}

RenderCommandListMT::RenderCommandListMT(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : CommandListMT<id<MTLRenderCommandEncoder>, RenderCommandListBase>(true, command_queue, render_pass)
{
    META_FUNCTION_TASK();
}

RenderCommandListMT::RenderCommandListMT(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListMT<id<MTLRenderCommandEncoder>, RenderCommandListBase>(false, parallel_render_command_list)
{
    META_FUNCTION_TASK();
}

void RenderCommandListMT::Reset(const Ptr<RenderState>& render_state_ptr, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    if (IsCommandEncoderInitialized())
    {
        RenderCommandListBase::Reset(render_state_ptr, p_debug_group);
        return;
    }

    if (IsParallel())
    {
        Ptr<ParallelRenderCommandListMT> parallel_render_cmd_list_ptr = std::static_pointer_cast<ParallelRenderCommandListMT>(GetParallelRenderCommandList());
        META_CHECK_ARG_NOT_NULL(parallel_render_cmd_list_ptr);

        InitializeCommandEncoder([parallel_render_cmd_list_ptr->GetNativeCommandEncoder() renderCommandEncoder]);
    }
    else
    {
        // If command buffer was not created for current frame yet,
        // then render pass descriptor should be reset with new frame drawable
        MTLRenderPassDescriptor* mtl_render_pass = GetRenderPassMT().GetNativeDescriptor(!IsCommandBufferInitialized());
        META_CHECK_ARG_NOT_NULL(mtl_render_pass);

        id<MTLCommandBuffer>& mtl_cmd_buffer = InitializeCommandBuffer();
        InitializeCommandEncoder([mtl_cmd_buffer renderCommandEncoderWithDescriptor:mtl_render_pass]);
    }

    RenderCommandListBase::Reset(render_state_ptr, p_debug_group);
}

void RenderCommandListMT::SetVertexBuffers(BufferSet& vertex_buffers)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::SetVertexBuffers(vertex_buffers);

    DrawingState& drawing_state = GetDrawingState();
    if (!(drawing_state.changes & DrawingState::Changes::VertexBuffers))
        return;

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

    const BufferSetMT& metal_vertex_buffers = static_cast<const BufferSetMT&>(vertex_buffers);
    const std::vector<id<MTLBuffer>>& mtl_buffers = metal_vertex_buffers.GetNativeBuffers();
    const std::vector<NSUInteger>&    mtl_offsets = metal_vertex_buffers.GetNativeOffsets();
    const NSRange                     mtl_range{ 0U, metal_vertex_buffers.GetCount() };
    [mtl_cmd_encoder setVertexBuffers:mtl_buffers.data() offsets:mtl_offsets.data() withRange:mtl_range];

    drawing_state.changes &= ~DrawingState::Changes::VertexBuffers;
}

void RenderCommandListMT::DrawIndexed(Primitive primitive, Buffer& index_buffer,
                                      uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    
    const BufferMT& metal_index_buffer = static_cast<const BufferMT&>(index_buffer);
    if (index_count == 0)
    {
        index_count = metal_index_buffer.GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_buffer, index_count, start_index, start_vertex, instance_count, start_instance);
    
    const MTLPrimitiveType mtl_primitive_type   = PrimitiveTypeToMetal(primitive);
    const MTLIndexType     mtl_index_type       = metal_index_buffer.GetNativeIndexType();
    const id<MTLBuffer>&   mtl_index_buffer     = metal_index_buffer.GetNativeBuffer();
    const uint32_t         mtl_index_stride     = mtl_index_type == MTLIndexTypeUInt32 ? 4 : 2;

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

    [mtl_cmd_encoder drawIndexedPrimitives: mtl_primitive_type
                                indexCount: index_count
                                 indexType: mtl_index_type
                               indexBuffer: mtl_index_buffer
                         indexBufferOffset: start_index * mtl_index_stride
                             instanceCount: instance_count
                                baseVertex: start_vertex
                              baseInstance: start_instance];
}

void RenderCommandListMT::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    const MTLPrimitiveType mtl_primitive_type = PrimitiveTypeToMetal(primitive);

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

    [mtl_cmd_encoder drawPrimitives: mtl_primitive_type
                        vertexStart: start_vertex
                        vertexCount: vertex_count
                      instanceCount: instance_count
                       baseInstance: start_instance];
}

RenderPassMT& RenderCommandListMT::GetRenderPassMT()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPassMT&>(GetPass());
}

} // namespace Methane::Graphics
