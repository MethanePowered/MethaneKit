/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/RenderCommandList.cpp
Vulkan implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/ParallelRenderCommandList.h>
#include <Methane/Graphics/Vulkan/RenderState.h>
#include <Methane/Graphics/Vulkan/RenderPattern.h>
#include <Methane/Graphics/Vulkan/RenderPass.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Buffer.h>
#include <Methane/Graphics/Vulkan/BufferSet.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Base
{

Ptr<Rhi::IRenderCommandList> RenderCommandList::CreateForSynchronization(Rhi::ICommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::RenderCommandList>(static_cast<Vulkan::CommandQueue&>(cmd_queue));
}

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::Vulkan
{

static vk::IndexType GetVulkanIndexTypeByStride(Data::Size index_stride_bytes)
{
    META_FUNCTION_TASK();
    switch(index_stride_bytes)
    {
    case 1: return vk::IndexType::eUint8EXT;
    case 2: return vk::IndexType::eUint16;
    case 4: return vk::IndexType::eUint32;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(index_stride_bytes, vk::IndexType::eNoneKHR, "unsupported index buffer stride size");
    }
}

static vk::CommandBufferInheritanceInfo CreateCommandBufferInheritInfo(const RenderPass& render_pass) noexcept
{
    META_FUNCTION_TASK();
    return vk::CommandBufferInheritanceInfo(
        render_pass.GetVulkanPattern().GetNativeRenderPass(),
        0U, // sub-pass
        render_pass.GetNativeFrameBuffer()
    );
}

RenderCommandList::RenderCommandList(CommandQueue& command_queue)
    : CommandList(vk::CommandBufferInheritanceInfo(), command_queue)
    , m_is_dynamic_state_supported(GetVulkanCommandQueue().GetVulkanDevice().IsDynamicStateSupported())
{ }

RenderCommandList::RenderCommandList(CommandQueue& command_queue, RenderPass& render_pass)
    : CommandList(CreateCommandBufferInheritInfo(render_pass), command_queue, render_pass)
    , m_is_dynamic_state_supported(GetVulkanCommandQueue().GetVulkanDevice().IsDynamicStateSupported())
{
    META_FUNCTION_TASK();
    static_cast<Data::IEmitter<IRenderPassCallback>&>(render_pass).Connect(*this);
}

RenderCommandList::RenderCommandList(ParallelRenderCommandList& parallel_render_command_list, bool is_beginning_cmd_list)
    : CommandList(CreateCommandBufferInheritInfo(parallel_render_command_list.GetVulkanRenderPass()), parallel_render_command_list, is_beginning_cmd_list)
    , m_is_dynamic_state_supported(GetVulkanCommandQueue().GetVulkanDevice().IsDynamicStateSupported())
{
    META_FUNCTION_TASK();
}

void RenderCommandList::Reset(IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    CommandList::ResetCommandState();
    CommandList::Reset(debug_group_ptr);
}

void RenderCommandList::ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    CommandList::ResetCommandState();
    CommandList::Reset(debug_group_ptr);
    CommandList::SetRenderState(render_state);
}

bool RenderCommandList::SetVertexBuffers(Rhi::IBufferSet& vertex_buffers, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!Base::RenderCommandList::SetVertexBuffers(vertex_buffers, set_resource_barriers))
        return false;

    const auto& vk_vertex_buffers = static_cast<const BufferSet&>(vertex_buffers);
    auto& vk_vertex_buffer_set = static_cast<BufferSet&>(vertex_buffers);
    if (const Ptr<Rhi::IResourceBarriers>& buffer_set_setup_barriers_ptr = vk_vertex_buffer_set.GetSetupTransitionBarriers();
        set_resource_barriers && vk_vertex_buffer_set.SetState(Rhi::ResourceState::VertexBuffer) && buffer_set_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_set_setup_barriers_ptr);
    }

    GetNativeCommandBufferDefault().bindVertexBuffers(0U, vk_vertex_buffers.GetNativeBuffers(), vk_vertex_buffers.GetNativeOffsets());
    return true;
}

bool RenderCommandList::SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!Base::RenderCommandList::SetIndexBuffer(index_buffer, set_resource_barriers))
        return false;

    auto& vk_index_buffer = static_cast<Buffer&>(index_buffer);
    if (Ptr<Rhi::IResourceBarriers>& buffer_setup_barriers_ptr = vk_index_buffer.GetSetupTransitionBarriers();
        set_resource_barriers && vk_index_buffer.SetState(Rhi::ResourceState::IndexBuffer, buffer_setup_barriers_ptr) && buffer_setup_barriers_ptr)
    {
        SetResourceBarriers(*buffer_setup_barriers_ptr);
    }

    const vk::IndexType vk_index_type = GetVulkanIndexTypeByStride(index_buffer.GetSettings().item_stride_size);
    GetNativeCommandBufferDefault().bindIndexBuffer(vk_index_buffer.GetNativeResource(), 0U, vk_index_type);
    return true;
}

void RenderCommandList::DrawIndexed(Primitive primitive, uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                    uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    if (const DrawingState& drawing_state = GetDrawingState();
        index_count == 0 && drawing_state.index_buffer_ptr)
    {
        index_count = drawing_state.index_buffer_ptr->GetFormattedItemsCount();
    }

    Base::RenderCommandList::DrawIndexed(primitive, index_count, start_index, start_vertex, instance_count, start_instance);

    UpdatePrimitiveTopology(primitive);
    GetNativeCommandBufferDefault().drawIndexed(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandList::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                             uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    Base::RenderCommandList::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    UpdatePrimitiveTopology(primitive);
    GetNativeCommandBufferDefault().draw(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandList::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());

    if (!IsParallel())
    {
        CommitCommandBuffer(CommandBufferType::SecondaryRenderPass);

        auto render_pass_ptr = static_cast<RenderPass*>(GetPassPtr());
        if (render_pass_ptr)
            render_pass_ptr->Begin(*this);

        GetNativeCommandBuffer(CommandBufferType::Primary).executeCommands(GetNativeCommandBuffer(CommandBufferType::SecondaryRenderPass));

        if (render_pass_ptr)
            render_pass_ptr->End(*this);
    }

    CommandList::Commit();
}

void RenderCommandList::OnRenderPassUpdated(const Rhi::IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    UpdateCommandBufferInheritInfo<CommandBufferType::SecondaryRenderPass>(
        CreateCommandBufferInheritInfo(static_cast<const RenderPass&>(render_pass)),
        IsParallel());
}

void RenderCommandList::UpdatePrimitiveTopology(Primitive primitive)
{
    META_FUNCTION_TASK();
    DrawingState& drawing_state = GetDrawingState();
    if (!drawing_state.changes.HasAnyBit(DrawingState::Change::PrimitiveType))
        return;

    drawing_state.primitive_type_opt = primitive;

    if (m_is_dynamic_state_supported)
    {
        const vk::PrimitiveTopology vk_primitive_topology = RenderState::GetVulkanPrimitiveTopology(primitive);
        GetNativeCommandBufferDefault().setPrimitiveTopologyEXT(vk_primitive_topology);
        drawing_state.changes.SetBitOff(DrawingState::Change::PrimitiveType);
    }
}

RenderPass& RenderCommandList::GetVulkanPass()
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPass&>(GetPass());
}

} // namespace Methane::Graphics::Vulkan
