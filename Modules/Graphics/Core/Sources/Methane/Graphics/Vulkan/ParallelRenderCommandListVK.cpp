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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandListVK.cpp
Vulkan implementation of the render command list interface.

******************************************************************************/

#include "ParallelRenderCommandListVK.h"
#include "RenderPassVK.h"
#include "CommandQueueVK.h"
#include "RenderCommandListVK.h"
#include "ContextVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Ptr<ParallelRenderCommandList> ParallelRenderCommandList::Create(CommandQueue& command_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<ParallelRenderCommandListVK>(static_cast<CommandQueueBase&>(command_queue), static_cast<RenderPassBase&>(render_pass));
}

ParallelRenderCommandListVK::ParallelRenderCommandListVK(CommandQueueBase& command_queue, RenderPassBase& render_pass)
    : ParallelRenderCommandListBase(command_queue, render_pass)
    , m_primary_cmd_list(static_cast<CommandQueueVK&>(command_queue), static_cast<RenderPassVK&>(render_pass))
{
    META_FUNCTION_TASK();
}

bool ParallelRenderCommandListVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    return ParallelRenderCommandListBase::SetName(name);
}

void ParallelRenderCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    m_primary_cmd_list.Reset(p_debug_group);
    ParallelRenderCommandListBase::Reset(p_debug_group);
}

void ParallelRenderCommandListVK::ResetWithState(RenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    m_primary_cmd_list.Reset(p_debug_group);
    ParallelRenderCommandListBase::ResetWithState(render_state, p_debug_group);
}

void ParallelRenderCommandListVK::SetBeginningResourceBarriers(const Resource::Barriers&)
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED();
}

void ParallelRenderCommandListVK::SetEndingResourceBarriers(const Resource::Barriers&)
{
    META_FUNCTION_TASK();
    META_FUNCTION_NOT_IMPLEMENTED();
}

void ParallelRenderCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());

    m_primary_cmd_list.Commit();

    ParallelRenderCommandListBase::Commit();

    const vk::CommandBuffer&       vk_primary_cmd_buffer = m_primary_cmd_list.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary);
    std::vector<vk::CommandBuffer> vk_secondary_cmd_buffers;
    std::vector<vk::CommandBuffer> vk_render_pass_cmd_buffers;

    const Refs<RenderCommandList>& parallel_cmd_list_refs = GetParallelCommandLists();
    vk_secondary_cmd_buffers.reserve(parallel_cmd_list_refs.size());
    vk_render_pass_cmd_buffers.reserve(parallel_cmd_list_refs.size());

    for(const Ref<RenderCommandList>& parallel_cmd_list_ref : parallel_cmd_list_refs)
    {
        auto& parallel_cmd_list_vk = static_cast<RenderCommandListVK&>(parallel_cmd_list_ref.get());
        vk_secondary_cmd_buffers.emplace_back(parallel_cmd_list_vk.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary));
        vk_render_pass_cmd_buffers.emplace_back(parallel_cmd_list_vk.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::SecondaryRenderPass));
    }

    vk_primary_cmd_buffer.begin(vk::CommandBufferBeginInfo());
    vk_primary_cmd_buffer.executeCommands(vk_secondary_cmd_buffers);

    RenderPassVK& render_pass = GetPassVK();
    render_pass.Begin(m_primary_cmd_list);

    vk_primary_cmd_buffer.executeCommands(vk_render_pass_cmd_buffers);

    render_pass.End(m_primary_cmd_list);
    vk_primary_cmd_buffer.end();
}

void ParallelRenderCommandListVK::Execute(const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    m_primary_cmd_list.Execute();
    ParallelRenderCommandListBase::Execute(completed_callback);
}

void ParallelRenderCommandListVK::Complete()
{
    META_FUNCTION_TASK();
    m_primary_cmd_list.Complete();
    ParallelRenderCommandListBase::Complete();
}

CommandQueueVK& ParallelRenderCommandListVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(GetCommandQueue());
}

RenderPassVK& ParallelRenderCommandListVK::GetPassVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPassVK&>(GetPass());
}

} // namespace Methane::Graphics
