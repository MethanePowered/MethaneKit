/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/CommandListSet.cpp
Vulkan command list set implementation.

******************************************************************************/

#include <Methane/Graphics/Vulkan/CommandListSet.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/RenderContext.h>
#include <Methane/Graphics/Vulkan/RenderCommandList.h>
#include <Methane/Graphics/Vulkan/ParallelRenderCommandList.h>
#include <Methane/Graphics/Vulkan/Device.h>

#include <Methane/Graphics/RHI/IRenderCommandList.h>
#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Instrumentation.h>

#include <sstream>
#include <algorithm>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandListSet> Rhi::ICommandListSet::Create(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::CommandListSet>(command_list_refs, frame_index_opt);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

static vk::PipelineStageFlags GetFrameBufferRenderingWaitStages(const Refs<Rhi::ICommandList>& command_list_refs)
{
    META_FUNCTION_TASK();
    vk::PipelineStageFlags wait_stages {};
    for(const Ref<Rhi::ICommandList>& command_list_ref : command_list_refs)
    {
        if (command_list_ref.get().GetType() != Rhi::CommandListType::Render)
            continue;

        const auto& render_command_list = dynamic_cast<const RenderCommandList&>(command_list_ref.get());
        if (!render_command_list.HasPass())
            continue;

        for(const Rhi::TextureView& attach_location : render_command_list.GetRenderPass().GetSettings().attachments)
        {
            const Rhi::TextureType attach_texture_type = attach_location.GetTexture().GetSettings().type;
            if (attach_texture_type == Rhi::TextureType::FrameBuffer)
                wait_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
            if (attach_texture_type == Rhi::TextureType::DepthStencil)
                wait_stages |= vk::PipelineStageFlagBits::eVertexShader;
        }
    }
    return wait_stages;
}

CommandListSet::CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : Base::CommandListSet(command_list_refs, frame_index_opt)
    , m_vk_wait_frame_buffer_rendering_on_stages(GetFrameBufferRenderingWaitStages(command_list_refs))
    , m_vk_device(GetVulkanCommandQueue().GetVulkanContext().GetVulkanDevice().GetNativeDevice())
    , m_vk_unique_execution_completed_semaphore(m_vk_device.createSemaphoreUnique(vk::SemaphoreCreateInfo()))
    , m_vk_unique_execution_completed_fence(m_vk_device.createFenceUnique(vk::FenceCreateInfo()))
{
    META_FUNCTION_TASK();
    const Refs<Base::CommandList>& base_command_list_refs = GetBaseRefs();
    
    m_vk_command_buffers.reserve(command_list_refs.size());
    for (const Ref<Base::CommandList>& command_list_ref : base_command_list_refs)
    {
        const Base::CommandList& command_list = command_list_ref.get();
        const auto& vulkan_command_list = command_list.GetType() == Rhi::CommandListType::ParallelRender
                                        ? static_cast<const ParallelRenderCommandList&>(command_list).GetVulkanPrimaryCommandList()
                                        : dynamic_cast<const Vulkan::ICommandList&>(command_list_ref.get());
        m_vk_command_buffers.emplace_back(vulkan_command_list.GetNativeCommandBuffer());
    }

    UpdateNativeDebugName();
}

void CommandListSet::Execute(const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    Base::CommandListSet::Execute(completed_callback);

    vk::SubmitInfo submit_info(
        GetWaitSemaphores(),
        GetWaitStages(),
        m_vk_command_buffers,
        GetNativeExecutionCompletedSemaphore()
    );
    
    Opt<vk::TimelineSemaphoreSubmitInfo> vk_timeline_submit_info_opt;
    if (const std::vector<uint64_t>& vk_wait_values = CommandListSet::GetWaitValues();
        !vk_wait_values.empty())
    {
        META_CHECK_ARG_EQUAL(vk_wait_values.size(), submit_info.waitSemaphoreCount);
        vk_timeline_submit_info_opt.emplace(vk_wait_values);
        submit_info.setPNext(&vk_timeline_submit_info_opt.value());
    }

    std::scoped_lock fence_guard(m_vk_unique_execution_completed_fence_mutex);
    const vk::Fence& vk_fence = GetNativeExecutionCompletedFence();
    if (m_signalled_execution_completed_fence)
    {
        // Do not reset not-signalled fence to workaround crash in validation layer on MacOS
        // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/4974
        m_vk_device.resetFences(vk_fence);
    }
    GetVulkanCommandQueue().GetNativeQueue().submit(submit_info, vk_fence);
    m_signalled_execution_completed_fence = true;
}

void CommandListSet::WaitUntilCompleted()
{
    META_FUNCTION_TASK();
    std::scoped_lock fence_guard(m_vk_unique_execution_completed_fence_mutex);
    const vk::Result execution_completed_fence_wait_result = m_vk_device.waitForFences(
        GetNativeExecutionCompletedFence(),
        true, std::numeric_limits<uint64_t>::max()
    );
    META_CHECK_ARG_EQUAL_DESCR(execution_completed_fence_wait_result, vk::Result::eSuccess, "failed to wait for command list set execution complete");
    Complete();
}

CommandQueue& CommandListSet::GetVulkanCommandQueue() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetBaseCommandQueue());
}

const CommandQueue& CommandListSet::GetVulkanCommandQueue() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const CommandQueue&>(GetBaseCommandQueue());
}

const std::vector<vk::Semaphore>& CommandListSet::GetWaitSemaphores()
{
    META_FUNCTION_TASK();
    const CommandQueue& command_queue = GetVulkanCommandQueue();
    const std::vector<vk::Semaphore>& vk_wait_semaphores = command_queue.GetWaitBeforeExecuting().semaphores;
    if (!m_vk_wait_frame_buffer_rendering_on_stages)
        return vk_wait_semaphores;

    m_vk_wait_semaphores = vk_wait_semaphores;
    m_vk_wait_semaphores.push_back(dynamic_cast<const RenderContext&>(command_queue.GetVulkanContext()).GetNativeFrameImageAvailableSemaphore());
    return m_vk_wait_semaphores;
}

const std::vector<vk::PipelineStageFlags>& CommandListSet::GetWaitStages()
{
    META_FUNCTION_TASK();
    const CommandQueue& command_queue = GetVulkanCommandQueue();
    const std::vector<vk::PipelineStageFlags>& vk_wait_stages = command_queue.GetWaitBeforeExecuting().stages;
    if (!m_vk_wait_frame_buffer_rendering_on_stages)
        return vk_wait_stages;

    m_vk_wait_stages = vk_wait_stages;
    m_vk_wait_stages.push_back(m_vk_wait_frame_buffer_rendering_on_stages);
    return m_vk_wait_stages;
}

const std::vector<uint64_t>& CommandListSet::GetWaitValues()
{
    META_FUNCTION_TASK();
    const CommandQueue& command_queue = GetVulkanCommandQueue();
    const CommandQueue::WaitInfo& wait_before_exec = command_queue.GetWaitBeforeExecuting();
    META_CHECK_ARG_EQUAL(wait_before_exec.wait_values.size(), wait_before_exec.semaphores.size());

    const std::vector<uint64_t>& vk_wait_values = wait_before_exec.wait_values;
    if (!m_vk_wait_frame_buffer_rendering_on_stages || vk_wait_values.empty())
        return vk_wait_values;

    m_vk_wait_values = vk_wait_values;
    m_vk_wait_values.push_back(0U);
    return m_vk_wait_values;
}

void CommandListSet::OnObjectNameChanged(Rhi::IObject& object, const std::string& old_name)
{
    META_FUNCTION_TASK();
    Base::CommandListSet::OnObjectNameChanged(object, old_name);
    UpdateNativeDebugName();
}

void CommandListSet::UpdateNativeDebugName()
{
    META_FUNCTION_TASK();
    const std::string execution_completed_name = fmt::format("{} Execution Completed", GetCombinedName());
    SetVulkanObjectName(m_vk_device, m_vk_unique_execution_completed_semaphore.get(), execution_completed_name);
    SetVulkanObjectName(m_vk_device, m_vk_unique_execution_completed_fence.get(), execution_completed_name);
}

} // namespace Methane::Graphics::Vulkan
