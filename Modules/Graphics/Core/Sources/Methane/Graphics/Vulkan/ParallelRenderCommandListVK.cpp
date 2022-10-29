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
    return std::make_shared<ParallelRenderCommandListVK>(static_cast<CommandQueueVK&>(command_queue), static_cast<RenderPassVK&>(render_pass));
}

ParallelRenderCommandListVK::ParallelRenderCommandListVK(CommandQueueVK& command_queue, RenderPassVK& render_pass)
    : ParallelRenderCommandListBase(command_queue, render_pass)
    , m_beginning_command_list(*this, true)
    , m_vk_ending_inheritance_info(render_pass.GetPatternVK().GetNativeRenderPass(), 0U, render_pass.GetNativeFrameBuffer())
    , m_ending_command_list(vk::CommandBufferLevel::eSecondary, // Ending command list creates Primary command buffer with Secondary level
                            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &m_vk_ending_inheritance_info),
                            static_cast<CommandQueueVK&>(command_queue), CommandList::Type::Render)
{
    META_FUNCTION_TASK();
}

bool ParallelRenderCommandListVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!ParallelRenderCommandListBase::SetName(name))
        return false;

    m_beginning_command_list.SetName(name);
    m_ending_command_list.SetName(GetTrailingCommandListDebugName(name, false));
    return true;
}

void ParallelRenderCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Reset(p_debug_group);

    if (p_debug_group)
    {
        // Instead of closing debug group in beginning CL commit, we force to close it in ending CL
        m_beginning_command_list.ClearOpenDebugGroups();

        m_ending_command_list.ResetOnce();
        m_ending_command_list.PushOpenDebugGroup(*p_debug_group);
    }

    ParallelRenderCommandListBase::Reset(p_debug_group);
}

void ParallelRenderCommandListVK::ResetWithState(IRenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();

    m_beginning_command_list.Reset(p_debug_group);

    if (p_debug_group)
    {
        // Instead of closing debug group in beginning CL commit, we force to close it in ending CL
        m_beginning_command_list.ClearOpenDebugGroups();

        m_ending_command_list.ResetOnce();
        m_ending_command_list.PushOpenDebugGroup(*p_debug_group);
    }

    ParallelRenderCommandListBase::ResetWithState(render_state, p_debug_group);
}

void ParallelRenderCommandListVK::SetBeginningResourceBarriers(const IResourceBarriers& resource_barriers)
{
    META_FUNCTION_TASK();
    m_ending_command_list.ResetOnce();
    m_beginning_command_list.SetResourceBarriers(resource_barriers);
}

void ParallelRenderCommandListVK::SetEndingResourceBarriers(const IResourceBarriers& resource_barriers)
{
    META_FUNCTION_TASK();
    m_ending_command_list.ResetOnce();
    m_ending_command_list.SetResourceBarriers(resource_barriers);
}

void ParallelRenderCommandListVK::SetParallelCommandListsCount(uint32_t count)
{
    META_FUNCTION_TASK();
    ParallelRenderCommandListBase::SetParallelCommandListsCount(count);

    m_vk_parallel_sync_cmd_buffers.clear();
    m_vk_parallel_pass_cmd_buffers.clear();

    const Refs<RenderCommandList>& parallel_cmd_list_refs = GetParallelCommandLists();
    m_vk_parallel_sync_cmd_buffers.reserve(parallel_cmd_list_refs.size());
    m_vk_parallel_pass_cmd_buffers.reserve(parallel_cmd_list_refs.size());

    for(const Ref<RenderCommandList>& parallel_cmd_list_ref : parallel_cmd_list_refs)
    {
        const auto& parallel_cmd_list_vk = static_cast<const RenderCommandListVK&>(parallel_cmd_list_ref.get());
        m_vk_parallel_sync_cmd_buffers.emplace_back(parallel_cmd_list_vk.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary));
        m_vk_parallel_pass_cmd_buffers.emplace_back(parallel_cmd_list_vk.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::SecondaryRenderPass));
    }
}

void ParallelRenderCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());
    ParallelRenderCommandListBase::Commit();

    const vk::CommandBuffer& vk_beginning_primary_cmd_buffer = m_beginning_command_list.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary);
    vk_beginning_primary_cmd_buffer.executeCommands(m_vk_parallel_sync_cmd_buffers);

    RenderPassVK& render_pass = GetPassVK();
    render_pass.Begin(m_beginning_command_list);

    vk_beginning_primary_cmd_buffer.executeCommands(m_vk_parallel_pass_cmd_buffers);

    render_pass.End(m_beginning_command_list);

    if (m_ending_command_list.GetState() == CommandList::State::Encoding)
    {
        m_ending_command_list.Commit();
        const vk::CommandBuffer& vk_ending_secondary_cmd_buffer = m_ending_command_list.GetNativeCommandBuffer(ICommandListVK::CommandBufferType::Primary);
        vk_beginning_primary_cmd_buffer.executeCommands(vk_ending_secondary_cmd_buffer);
    }

    m_beginning_command_list.Commit();
}

void ParallelRenderCommandListVK::Execute(const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Execute();
    ParallelRenderCommandListBase::Execute(completed_callback);
    if (m_ending_command_list.GetState() == CommandList::State::Committed)
        m_ending_command_list.Execute();
}

void ParallelRenderCommandListVK::Complete()
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Complete();
    ParallelRenderCommandListBase::Complete();
    if (m_ending_command_list.GetState() == CommandList::State::Executing)
        m_ending_command_list.Complete();
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
