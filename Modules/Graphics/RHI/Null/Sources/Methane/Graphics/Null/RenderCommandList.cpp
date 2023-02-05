/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/RenderCommandList.cpp
Null implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/Null/RenderCommandList.h>
#include <Methane/Graphics/Null/CommandQueue.h>
#include <Methane/Graphics/Null/ParallelRenderCommandList.h>
#include <Methane/Graphics/Null/Buffer.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Base
{

Ptr<Rhi::IRenderCommandList> RenderCommandList::CreateForSynchronization(Rhi::ICommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<Null::RenderCommandList>(dynamic_cast<Null::CommandQueue&>(cmd_queue));
}

} // namespace Methane::Graphics::Base

namespace Methane::Graphics::Null
{

RenderCommandList::RenderCommandList(CommandQueue& command_queue)
    : CommandList(command_queue)
{ }

RenderCommandList::RenderCommandList(CommandQueue& command_queue, RenderPass& render_pass)
    : CommandList(command_queue, render_pass)
{ }

RenderCommandList::RenderCommandList(ParallelRenderCommandList& parallel_render_command_list)
    : CommandList(parallel_render_command_list)
{ }

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

    return true;
}

bool RenderCommandList::SetIndexBuffer(Rhi::IBuffer& index_buffer, bool set_resource_barriers)
{
    META_FUNCTION_TASK();
    if (!Base::RenderCommandList::SetIndexBuffer(index_buffer, set_resource_barriers))
        return false;

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
}

void RenderCommandList::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    Base::RenderCommandList::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);
}

} // namespace Methane::Graphics::Null
