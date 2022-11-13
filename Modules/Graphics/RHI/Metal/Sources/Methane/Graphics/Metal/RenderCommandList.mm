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

FILE: Methane/Graphics/Metal/RenderCommandList.mm
Metal implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/Metal/CommandQueue.hh>
#include <Methane/Graphics/Metal/RenderCommandList.hh>
#include <Methane/Graphics/Metal/ParallelRenderCommandList.hh>
#include <Methane/Graphics/Metal/RenderPass.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Graphics/Metal/Buffer.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IRenderCommandList> IRenderCommandList::Create(ICommandQueue& command_queue, IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::RenderCommandList>(dynamic_cast<Metal::CommandQueue&>(command_queue), static_cast<Base::RenderPass&>(render_pass));
}

Ptr<IRenderCommandList> IRenderCommandList::Create(IParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::RenderCommandList>(dynamic_cast<Metal::ParallelRenderCommandList&>(parallel_render_command_list));
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

Ptr<Rhi::IRenderCommandList> Base::RenderCommandList::CreateForSynchronization(Rhi::ICommandQueue&)
{
    META_FUNCTION_TASK();
    return nullptr;
}

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::Metal
{

static MTLPrimitiveType PrimitiveTypeToMetal(Rhi::RenderPrimitive primitive) noexcept
{
    META_FUNCTION_TASK();
    switch (primitive)
    {
        case Rhi::RenderPrimitive::Point:          return MTLPrimitiveTypePoint;
        case Rhi::RenderPrimitive::Line:           return MTLPrimitiveTypeLine;
        case Rhi::RenderPrimitive::LineStrip:      return MTLPrimitiveTypeLineStrip;
        case Rhi::RenderPrimitive::Triangle:       return MTLPrimitiveTypeTriangle;
        case Rhi::RenderPrimitive::TriangleStrip:  return MTLPrimitiveTypeTriangleStrip;
    }
    return MTLPrimitiveTypeTriangleStrip;
}

static bool GetDeviceSupportOfGpuFamilyApple3(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    const id<MTLDevice> mtl_device = command_queue.GetMetalContext().GetMetalDevice().GetNativeDevice();
    return [mtl_device supportsFamily:MTLGPUFamilyApple3] ||
           [mtl_device supportsFamily:MTLGPUFamilyMac2];
}

RenderCommandList::RenderCommandList(CommandQueue& command_queue, Base::RenderPass& render_pass)
    : CommandList<id<MTLRenderCommandEncoder>, Base::RenderCommandList>(true, command_queue, render_pass)
    , m_device_supports_gpu_family_apple_3(GetDeviceSupportOfGpuFamilyApple3(command_queue))
{
    META_FUNCTION_TASK();
}

RenderCommandList::RenderCommandList(ParallelRenderCommandList& parallel_render_command_list)
    : CommandList<id<MTLRenderCommandEncoder>, Base::RenderCommandList>(false, parallel_render_command_list)
    , m_parallel_render_command_list_ptr(&parallel_render_command_list)
    , m_device_supports_gpu_family_apple_3(GetDeviceSupportOfGpuFamilyApple3(parallel_render_command_list.GetMetalCommandQueue()))
{
    META_FUNCTION_TASK();
}

void RenderCommandList::Reset(IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetCommandEncoder();
    Base::RenderCommandList::Reset(p_debug_group);
}

void RenderCommandList::ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetCommandEncoder();
    Base::RenderCommandList::ResetWithState(render_state, p_debug_group);
}

void RenderCommandList::ResetCommandEncoder()
{
    META_FUNCTION_TASK();
    if (IsCommandEncoderInitialized())
        return;

    if (IsParallel())
    {
        META_CHECK_ARG_NOT_NULL(m_parallel_render_command_list_ptr);
        InitializeCommandEncoder([m_parallel_render_command_list_ptr->GetNativeCommandEncoder() renderCommandEncoder]);
    }
    else
    {
        // If command buffer was not created for current frame yet,
        // then render pass descriptor should be reset with new frame drawable
        MTLRenderPassDescriptor* mtl_render_pass = GetMetalRenderPass().GetNativeDescriptor(!IsCommandBufferInitialized());
        META_CHECK_ARG_NOT_NULL(mtl_render_pass);

        const id<MTLCommandBuffer>& mtl_cmd_buffer = InitializeCommandBuffer();
        InitializeCommandEncoder([mtl_cmd_buffer renderCommandEncoderWithDescriptor:mtl_render_pass]);
    }
}

bool RenderCommandList::SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!Base::RenderCommandList::SetVertexBuffers(vertex_buffers, set_resource_barriers))
        return false;

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

    const BufferSet& metal_vertex_buffers = static_cast<const BufferSet&>(vertex_buffers);
    const std::vector<id<MTLBuffer>>& mtl_buffers = metal_vertex_buffers.GetNativeBuffers();
    const std::vector<NSUInteger>&    mtl_offsets = metal_vertex_buffers.GetNativeOffsets();
    const NSRange                     mtl_range{ 0U, metal_vertex_buffers.GetCount() };
    [mtl_cmd_encoder setVertexBuffers:mtl_buffers.data() offsets:mtl_offsets.data() withRange:mtl_range];

    return true;
}

void RenderCommandList::DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();

    DrawingState& drawing_state = GetDrawingState();
    if (index_count == 0 && drawing_state.index_buffer_ptr)
    {
        index_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
    }

    Base::RenderCommandList::DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);

    const Buffer& metal_index_buffer = static_cast<const Buffer&>(*drawing_state.index_buffer_ptr);
    const MTLPrimitiveType mtl_primitive_type = PrimitiveTypeToMetal(primitive);
    const MTLIndexType     mtl_index_type     = metal_index_buffer.GetNativeIndexType();
    const id <MTLBuffer>& mtl_index_buffer = metal_index_buffer.GetNativeBuffer();
    const uint32_t mtl_index_stride = mtl_index_type == MTLIndexTypeUInt32 ? 4 : 2;

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

    if (m_device_supports_gpu_family_apple_3)
    {
        [mtl_cmd_encoder drawIndexedPrimitives:mtl_primitive_type
                                    indexCount:index_count
                                     indexType:mtl_index_type
                                   indexBuffer:mtl_index_buffer
                             indexBufferOffset:start_index * mtl_index_stride
                                 instanceCount:instance_count
                                    baseVertex:start_vertex
                                  baseInstance:start_instance];
    }
    else
    {
        [mtl_cmd_encoder drawIndexedPrimitives:mtl_primitive_type
                                    indexCount:index_count
                                     indexType:mtl_index_type
                                   indexBuffer:mtl_index_buffer
                             indexBufferOffset:start_index * mtl_index_stride
                                 instanceCount:instance_count];

        if (start_vertex > 0U || start_instance > 0U)
        {
            NSLog(@"DrawIndexed 'start_vertex' and 'start_instance' arguments are not supported on iOS devices with GPU Family < Apple-3");
        }
    }
}

void RenderCommandList::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    Base::RenderCommandList::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    const MTLPrimitiveType mtl_primitive_type = PrimitiveTypeToMetal(primitive);

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

    if (m_device_supports_gpu_family_apple_3)
    {
        [mtl_cmd_encoder drawPrimitives:mtl_primitive_type
                            vertexStart:start_vertex
                            vertexCount:vertex_count
                          instanceCount:instance_count
                           baseInstance:start_instance];
    }
    else
    {
        [mtl_cmd_encoder drawPrimitives:mtl_primitive_type
                            vertexStart:start_vertex
                            vertexCount:vertex_count
                          instanceCount:instance_count];

        if (start_instance > 0U)
        {
            NSLog(@"Draw 'start_instance' argument is not supported on iOS devices with GPU Family < Apple-3");
        }
    }
}

RenderPass& RenderCommandList::GetMetalRenderPass()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPass&>(GetPass());
}

} // namespace Methane::Graphics::Metal
