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

FILE: Methane/Graphics/Metal/RenderCommandListMT.mm
Metal implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListMT.hh"
#include "ParallelRenderCommandListMT.hh"
#include "RenderStateMT.hh"
#include "RenderPassMT.hh"
#include "CommandQueueMT.hh"
#include "ContextMT.hh"
#include "BufferMT.hh"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

static MTLPrimitiveType PrimitiveTypeToMetal(RenderCommandList::Primitive primitive) noexcept
{
    ITT_FUNCTION_TASK();

    switch (primitive)
    {
        case RenderCommandList::Primitive::Point:          return MTLPrimitiveTypePoint;
        case RenderCommandList::Primitive::Line:           return MTLPrimitiveTypeLine;
        case RenderCommandList::Primitive::LineStrip:      return MTLPrimitiveTypeLineStrip;
        case RenderCommandList::Primitive::Triangle:       return MTLPrimitiveTypeTriangle;
        case RenderCommandList::Primitive::TriangleStrip:  return MTLPrimitiveTypeTriangleStrip;
        default:                                           assert(0);
    }
    return MTLPrimitiveTypeTriangleStrip;
}

RenderCommandList::Ptr RenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListMT>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

RenderCommandList::Ptr RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<RenderCommandListMT>(static_cast<ParallelRenderCommandListBase&>(parallel_render_command_list));
}

RenderCommandListMT::RenderCommandListMT(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : RenderCommandListBase(command_queue, render_pass)
{
    ITT_FUNCTION_TASK();
}

RenderCommandListMT::RenderCommandListMT(ParallelRenderCommandListBase& parallel_render_command_list)
    : RenderCommandListBase(parallel_render_command_list)
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListMT::Reset(RenderState& render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();
    
    StartRenderEncoding();

    RenderCommandListBase::Reset(render_state, debug_group);
}

void RenderCommandListMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetName(name);
    
    NSString* ns_name = MacOS::ConvertToNSType<std::string, NSString*>(name);
    
    if (m_mtl_render_encoder != nil)
    {
        m_mtl_render_encoder.label = ns_name;
    }
    
    if (m_mtl_blit_encoder != nil)
    {
        m_mtl_blit_encoder.label = ns_name;
    }
    
    if (m_mtl_cmd_buffer != nil)
    {
        m_mtl_cmd_buffer.label = ns_name;
    }
}

void RenderCommandListMT::PushDebugGroup(const std::string& name)
{
    ITT_FUNCTION_TASK();

    assert(m_mtl_render_encoder != nil);
    NSString* ns_name = MacOS::ConvertToNSType<std::string, NSString*>(name);
    [m_mtl_render_encoder pushDebugGroup:ns_name];
}

void RenderCommandListMT::PopDebugGroup()
{
    ITT_FUNCTION_TASK();

    assert(m_mtl_render_encoder != nil);
    [m_mtl_render_encoder popDebugGroup];
}

void RenderCommandListMT::SetVertexBuffers(const Buffer::Refs& vertex_buffers)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::SetVertexBuffers(vertex_buffers);

    if (!m_draw_state.flags.vertex_buffers_changed)
        return;

    assert(m_mtl_render_encoder != nil);
    uint32_t vb_index = 0;
    for (auto vertex_buffer_ref : vertex_buffers)
    {
        assert(vertex_buffer_ref.get().GetBufferType() == Buffer::Type::Vertex);
        const BufferMT& metal_buffer = static_cast<const BufferMT&>(vertex_buffer_ref.get());
        [m_mtl_render_encoder setVertexBuffer:metal_buffer.GetNativeBuffer() offset:0 atIndex:vb_index];
        vb_index++;
    }
}

void RenderCommandListMT::DrawIndexed(Primitive primitive, Buffer& index_buffer,
                                      uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    ITT_FUNCTION_TASK();
    
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

    assert(m_mtl_render_encoder != nil);
    [m_mtl_render_encoder drawIndexedPrimitives: mtl_primitive_type
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
    ITT_FUNCTION_TASK();

    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    const MTLPrimitiveType mtl_primitive_type = PrimitiveTypeToMetal(primitive);

    assert(m_mtl_render_encoder != nil);
    [m_mtl_render_encoder drawPrimitives: mtl_primitive_type
                             vertexStart: start_vertex
                             vertexCount: vertex_count
                           instanceCount: instance_count
                            baseInstance: start_instance];
}

void RenderCommandListMT::Commit(bool present_drawable)
{
    ITT_FUNCTION_TASK();
    
    assert(!IsCommitted());

    RenderCommandListBase::Commit(present_drawable);

    EndBlitEncoding();
    EndRenderEncoding();

    if (!m_mtl_cmd_buffer || m_is_parallel)
        return;
    
    if (present_drawable)
    {
        [m_mtl_cmd_buffer presentDrawable: GetCommandQueueMT().GetContextMT().GetNativeDrawable()];
    }

    [m_mtl_cmd_buffer enqueue];
}

void RenderCommandListMT::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    RenderCommandListBase::Execute(frame_index);

    if (m_is_parallel || !m_mtl_cmd_buffer)
        return;

    [m_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
        Complete(frame_index);
    }];

    [m_mtl_cmd_buffer commit];
    m_mtl_cmd_buffer  = nil;
}

void RenderCommandListMT::InitializeCommandBuffer()
{
    ITT_FUNCTION_TASK();
    if (m_is_parallel || m_mtl_cmd_buffer != nil)
        return;

    m_mtl_cmd_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];
    assert(m_mtl_cmd_buffer != nil);
    
    m_mtl_cmd_buffer.label = MacOS::ConvertToNSType<std::string, NSString*>(GetName());
}
    
void RenderCommandListMT::StartRenderEncoding()
{
    ITT_FUNCTION_TASK();
    if (m_mtl_render_encoder != nil)
        return;
    
    EndBlitEncoding();
    
    // NOTE: If command buffer was not created for current frame yet,
    //       then render pass descriptor should be reset with new frame drawable
    MTLRenderPassDescriptor* mtl_render_pass = GetPassMT().GetNativeDescriptor(!m_is_parallel && m_mtl_cmd_buffer == nil);
    
    InitializeCommandBuffer();

    if (m_is_parallel)
    {
        ParallelRenderCommandListMT::Ptr sp_parallel_render_cmd_list = std::static_pointer_cast<ParallelRenderCommandListMT>(m_wp_parallel_render_command_list.lock());
        assert(!!sp_parallel_render_cmd_list);
        id<MTLParallelRenderCommandEncoder>& mtl_parallel_render_command_encoder = sp_parallel_render_cmd_list->GetNativeParallelRenderEncoder();
        m_mtl_render_encoder = [mtl_parallel_render_command_encoder renderCommandEncoder];
    }
    else
    {
        assert(!!mtl_render_pass);
        m_mtl_render_encoder = [m_mtl_cmd_buffer renderCommandEncoderWithDescriptor:mtl_render_pass];
    }
    
    assert(m_mtl_render_encoder != nil);
    m_mtl_render_encoder.label = MacOS::ConvertToNSType<std::string, NSString*>(GetName());
}

void RenderCommandListMT::EndRenderEncoding()
{
    ITT_FUNCTION_TASK();
    if (m_mtl_render_encoder == nil)
        return;

    [m_mtl_render_encoder endEncoding];
    m_mtl_render_encoder = nil;
}
    
void RenderCommandListMT::StartBlitEncoding()
{
    ITT_FUNCTION_TASK();
    if (m_mtl_blit_encoder != nil)
        return;

    InitializeCommandBuffer();
    EndRenderEncoding();
    
    assert(m_mtl_cmd_buffer != nil);
    m_mtl_blit_encoder = [m_mtl_cmd_buffer blitCommandEncoder];
    
    assert(m_mtl_blit_encoder != nil);
    m_mtl_blit_encoder.label = MacOS::ConvertToNSType<std::string, NSString*>(GetName());
}

void RenderCommandListMT::EndBlitEncoding()
{
    ITT_FUNCTION_TASK();
    if (m_mtl_blit_encoder == nil)
        return;

    [m_mtl_blit_encoder endEncoding];
    m_mtl_blit_encoder = nil;
}

CommandQueueMT& RenderCommandListMT::GetCommandQueueMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueMT&>(*m_sp_command_queue);
}

RenderPassMT& RenderCommandListMT::GetPassMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<class RenderPassMT&>(GetPass());
}

} // namespace Methane::Graphics
