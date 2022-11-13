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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandList.cpp
Vulkan implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ParallelRenderCommandList.h>
#include <Methane/Graphics/Vulkan/RenderPass.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>
#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/IContextVk.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IParallelRenderCommandList> IParallelRenderCommandList::Create(ICommandQueue& command_queue, IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::ParallelRenderCommandList>(static_cast<Vulkan::CommandQueue&>(command_queue),
                                                               static_cast<Vulkan::RenderPass&>(render_pass));
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

ParallelRenderCommandList::ParallelRenderCommandList(CommandQueue& command_queue, RenderPass& render_pass)
    : Base::ParallelRenderCommandList(command_queue, render_pass)
    , m_beginning_command_list(*this, true)
    , m_vk_ending_inheritance_info(render_pass.GetVulkanPattern().GetNativeRenderPass(), 0U, render_pass.GetNativeFrameBuffer())
    , m_ending_command_list(vk::CommandBufferLevel::eSecondary, // Ending command list creates Primary command buffer with Secondary level
                            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &m_vk_ending_inheritance_info),
                            static_cast<CommandQueue&>(command_queue), Rhi::CommandListType::Render)
{
    META_FUNCTION_TASK();
}

bool ParallelRenderCommandList::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Base::ParallelRenderCommandList::SetName(name))
        return false;

    m_beginning_command_list.SetName(name);
    m_ending_command_list.SetName(GetTrailingCommandListDebugName(name, false));
    return true;
}

void ParallelRenderCommandList::Reset(IDebugGroup* p_debug_group)
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

    Base::ParallelRenderCommandList::Reset(p_debug_group);
}

void ParallelRenderCommandList::ResetWithState(Rhi::IRenderState& render_state, IDebugGroup* p_debug_group)
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

    Base::ParallelRenderCommandList::ResetWithState(render_state, p_debug_group);
}

void ParallelRenderCommandList::SetBeginningResourceBarriers(const Rhi::IResourceBarriers& resource_barriers)
{
    META_FUNCTION_TASK();
    m_ending_command_list.ResetOnce();
    m_beginning_command_list.SetResourceBarriers(resource_barriers);
}

void ParallelRenderCommandList::SetEndingResourceBarriers(const Rhi::IResourceBarriers& resource_barriers)
{
    META_FUNCTION_TASK();
    m_ending_command_list.ResetOnce();
    m_ending_command_list.SetResourceBarriers(resource_barriers);
}

void ParallelRenderCommandList::SetParallelCommandListsCount(uint32_t count)
{
    META_FUNCTION_TASK();
    Base::ParallelRenderCommandList::SetParallelCommandListsCount(count);

    m_vk_parallel_sync_cmd_buffers.clear();
    m_vk_parallel_pass_cmd_buffers.clear();

    const Refs<Rhi::IRenderCommandList>& parallel_cmd_list_refs = GetParallelCommandLists();
    m_vk_parallel_sync_cmd_buffers.reserve(parallel_cmd_list_refs.size());
    m_vk_parallel_pass_cmd_buffers.reserve(parallel_cmd_list_refs.size());

    for(const Ref<Rhi::IRenderCommandList>& parallel_cmd_list_ref : parallel_cmd_list_refs)
    {
        const auto& parallel_cmd_list_vk = static_cast<const RenderCommandList&>(parallel_cmd_list_ref.get());
        m_vk_parallel_sync_cmd_buffers.emplace_back(parallel_cmd_list_vk.GetNativeCommandBuffer(ICommandListVk::CommandBufferType::Primary));
        m_vk_parallel_pass_cmd_buffers.emplace_back(parallel_cmd_list_vk.GetNativeCommandBuffer(ICommandListVk::CommandBufferType::SecondaryRenderPass));
    }
}

void ParallelRenderCommandList::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());
    Base::ParallelRenderCommandList::Commit();

    const vk::CommandBuffer& vk_beginning_primary_cmd_buffer = m_beginning_command_list.GetNativeCommandBuffer(ICommandListVk::CommandBufferType::Primary);
    vk_beginning_primary_cmd_buffer.executeCommands(m_vk_parallel_sync_cmd_buffers);

    RenderPass& render_pass = GetVulkanPass();
    render_pass.Begin(m_beginning_command_list);

    vk_beginning_primary_cmd_buffer.executeCommands(m_vk_parallel_pass_cmd_buffers);

    render_pass.End(m_beginning_command_list);

    if (m_ending_command_list.GetState() == Rhi::CommandListState::Encoding)
    {
        m_ending_command_list.Commit();
        const vk::CommandBuffer& vk_ending_secondary_cmd_buffer = m_ending_command_list.GetNativeCommandBuffer(ICommandListVk::CommandBufferType::Primary);
        vk_beginning_primary_cmd_buffer.executeCommands(vk_ending_secondary_cmd_buffer);
    }

    m_beginning_command_list.Commit();
}

void ParallelRenderCommandList::Execute(const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Execute();
    Base::ParallelRenderCommandList::Execute(completed_callback);
    if (m_ending_command_list.GetState() == Rhi::CommandListState::Committed)
        m_ending_command_list.Execute();
}

void ParallelRenderCommandList::Complete()
{
    META_FUNCTION_TASK();
    m_beginning_command_list.Complete();
    Base::ParallelRenderCommandList::Complete();
    if (m_ending_command_list.GetState() == Rhi::CommandListState::Executing)
        m_ending_command_list.Complete();
}

CommandQueue& ParallelRenderCommandList::GetVulkanCommandQueue() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class CommandQueue&>(GetCommandQueue());
}

RenderPass& ParallelRenderCommandList::GetVulkanPass() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class RenderPass&>(GetPass());
}

} // namespace Methane::Graphics::Vulkan
