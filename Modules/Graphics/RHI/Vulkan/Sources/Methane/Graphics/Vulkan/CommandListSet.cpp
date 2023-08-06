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

static Rhi::IRenderPass* GetRenderPassFromCommandList(const Rhi::ICommandList& command_list)
{
    META_FUNCTION_TASK();


    switch(const Rhi::CommandListType cmd_list_type = command_list.GetType();
           cmd_list_type)
    {
    case Rhi::CommandListType::Render:
    {
        const auto& render_cmd_list = dynamic_cast<const RenderCommandList&>(command_list);
        if (render_cmd_list.HasPass())
            return &render_cmd_list.GetRenderPass();
    } break;

    case Rhi::CommandListType::ParallelRender:
    {
        const auto& parallel_render_cmd_list = dynamic_cast<const ParallelRenderCommandList&>(command_list);
        return &parallel_render_cmd_list.GetRenderPass();
    } break;

    default:
        break;
    }

    return nullptr;
}

static vk::PipelineStageFlags GetFrameBufferRenderingWaitStages(const Refs<Rhi::ICommandList>& command_list_refs)
{
    META_FUNCTION_TASK();
    vk::PipelineStageFlags wait_stages {};
    for(const Ref<Rhi::ICommandList>& command_list_ref : command_list_refs)
    {
        const Rhi::IRenderPass* render_pass_ptr = GetRenderPassFromCommandList(command_list_ref.get());
        if (!render_pass_ptr)
            continue;

        for(const Rhi::TextureView& attach_location : render_pass_ptr->GetSettings().attachments)
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

    auto [vk_submit_info, vk_timeline_semaphore_submit_info] = GetSubmitInfo();

    // FIXME: MoltenVK is crashing on Apple platforms on attempt to use submit info with timeline semaphore values,
    //        while timeline semaphore extension is properly enabled in Device.cpp and this code works fine on Linux.
#ifndef __APPLE__
    if (vk_timeline_semaphore_submit_info.waitSemaphoreValueCount ||
        vk_timeline_semaphore_submit_info.signalSemaphoreValueCount)
    {
        // Bind vk::TimelineSemaphoreSubmitInfo to the vk::SubmitInfo
        vk_submit_info.setPNext(&vk_timeline_semaphore_submit_info);
    }
#endif

    std::scoped_lock fence_guard(m_execution_completed_fence_mutex);
    if (m_signalled_execution_completed_fence)
    {
        // Do not reset not-signalled fence to workaround crash in validation layer on MacOS
        // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/4974
        m_vk_device.resetFences(m_vk_unique_execution_completed_fence.get());
    }

    GetVulkanCommandQueue().GetNativeQueue().submit(vk_submit_info, m_vk_unique_execution_completed_fence.get());
    m_signalled_execution_completed_fence = true;
}

void CommandListSet::WaitUntilCompleted()
{
    META_FUNCTION_TASK();
    std::scoped_lock fence_guard(m_execution_completed_fence_mutex);
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

CommandListSet::SubmitInfo CommandListSet::GetSubmitInfo()
{
    META_FUNCTION_TASK();
    const CommandQueue& command_queue = GetVulkanCommandQueue();
    const CommandQueue::WaitInfo& wait_before_exec = command_queue.GetWaitBeforeExecuting();

    const std::vector<vk::Semaphore>&          vk_wait_semaphores = m_vk_wait_frame_buffer_rendering_on_stages ? m_vk_wait_semaphores : wait_before_exec.semaphores;
    const std::vector<uint64_t>&               vk_wait_values     = m_vk_wait_frame_buffer_rendering_on_stages ? m_vk_wait_values     : wait_before_exec.values;
    const std::vector<vk::PipelineStageFlags>& vk_wait_stages     = m_vk_wait_frame_buffer_rendering_on_stages ? m_vk_wait_stages     : wait_before_exec.stages;

    if (m_vk_wait_frame_buffer_rendering_on_stages)
    {
        m_vk_wait_semaphores = wait_before_exec.semaphores;
        m_vk_wait_values     = wait_before_exec.values;
        m_vk_wait_stages     = wait_before_exec.stages;

        const auto& render_context = dynamic_cast<const RenderContext&>(command_queue.GetVulkanContext());
        const vk::Semaphore& frame_image_available_semaphore = render_context.GetNativeFrameImageAvailableSemaphore(GetFrameIndex());
        if (frame_image_available_semaphore)
        {
            m_vk_wait_semaphores.push_back(frame_image_available_semaphore);
            m_vk_wait_values.push_back(0U);
            m_vk_wait_stages.push_back(m_vk_wait_frame_buffer_rendering_on_stages);
        }
    }

    META_CHECK_ARG_EQUAL_DESCR(vk_wait_semaphores.size(), vk_wait_stages.size(), "number of wait semaphores and stages must be equal");
    SubmitInfo submit_info{};
    submit_info.first = vk::SubmitInfo(
        vk_wait_semaphores,
        vk_wait_stages,
        m_vk_command_buffers,
        m_vk_unique_execution_completed_semaphore.get()
    );

    if (!vk_wait_values.empty())
    {
        META_CHECK_ARG_EQUAL_DESCR(vk_wait_semaphores.size(), vk_wait_values.size(), "number of wait timeline semaphores and values must be equal");
        submit_info.second = vk::TimelineSemaphoreSubmitInfo(vk_wait_values);
    }

    return submit_info;
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
