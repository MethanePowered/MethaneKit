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

FILE: Methane/Graphics/RenderCommandListBase.cpp
Base implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListBase.h"
#include "RenderPassBase.h"
#include "RenderStateBase.h"
#include "BufferBase.h"
#include "Instrumentation.h"

#include <cassert>

namespace Methane
{
namespace Graphics
{

RenderCommandListBase::RenderCommandListBase(CommandQueueBase& command_queue, RenderPassBase& pass)
    : CommandListBase(command_queue)
    , m_sp_pass(pass.GetPtr())
{
    ITT_FUNCTION_TASK();
}

void RenderCommandListBase::Reset(RenderState& render_state, const std::string& debug_group)
{
    ITT_FUNCTION_TASK();
    if (!debug_group.empty())
    {
        PushDebugGroup(debug_group);
        m_pop_debug_group_on_commit = true;
    }
    
    static_cast<RenderStateBase&>(render_state).Apply(*this);
}

void RenderCommandListBase::SetVertexBuffers(const Buffer::Refs& vertex_buffers)
{
    ITT_FUNCTION_TASK();
    if (vertex_buffers.empty())
    {
        throw std::invalid_argument("Can not set empty vertex buffers.");
    }
}

void RenderCommandListBase::DrawIndexed(Primitive /*primitive_type*/, const Buffer& index_buffer, uint32_t instance_count)
{
    ITT_FUNCTION_TASK();
    if (index_buffer.GetBufferType() != Buffer::Type::Index)
    {
        throw std::invalid_argument("Can not draw with index buffer of wrong type \"" + static_cast<const BufferBase&>(index_buffer).GetBufferTypeName() + "\". BufferBase of \"Index\" type is expected.");
    }
    if (index_buffer.GetFormattedItemsCount() == 0)
    {
        throw std::invalid_argument("Can not draw with index buffer which cotains no formatted indices.");
    }
    if (instance_count == 0)
    {
        throw std::invalid_argument("Can not draw zero instances.");
    }
}

void RenderCommandListBase::Draw(Primitive /*primitive_type*/ , uint32_t vertex_count, uint32_t instance_count)
{
    ITT_FUNCTION_TASK();
    if (vertex_count == 0)
    {
        throw std::invalid_argument("Can not draw zero vertices.");
    }
    if (instance_count == 0)
    {
        throw std::invalid_argument("Can not draw zero instances.");
    }
}

RenderPassBase& RenderCommandListBase::GetPass()
{
    ITT_FUNCTION_TASK();
    assert(!!m_sp_pass);
    return static_cast<RenderPassBase&>(*m_sp_pass);
}

} // namespace Graphics
} // namespace Methane
