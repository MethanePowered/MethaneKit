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

FILE: Methane/Graphics/Vulkan/CommandListVK.cpp
Vulkan command lists sequence implementation

******************************************************************************/

#include "CommandListVK.h"
#include "CommandQueueVK.h"
#include "ContextVK.h"
#include "RenderContextVK.h"
#include "RenderCommandListVK.h"
#include "ParallelRenderCommandListVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/IRenderPass.h>
#include <Methane/Instrumentation.h>

#include <sstream>
#include <algorithm>

namespace Methane::Graphics
{

static vk::PipelineStageFlags GetFrameBufferRenderingWaitStages(const Refs<CommandList>& command_list_refs)
{
    META_FUNCTION_TASK();
    vk::PipelineStageFlags wait_stages {};
    for(const Ref<CommandList>& command_list_ref : command_list_refs)
    {
        if (command_list_ref.get().GetType() != CommandList::Type::Render)
            continue;

        const auto& render_command_list = dynamic_cast<const RenderCommandListVK&>(command_list_ref.get());
        if (!render_command_list.HasPass())
            continue;

        for(const ITexture::View& attach_location : render_command_list.GetRenderPass().GetSettings().attachments)
        {
            const ITexture::Type attach_texture_type = attach_location.GetTexture().GetSettings().type;
            if (attach_texture_type == ITexture::Type::FrameBuffer)
                wait_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
            if (attach_texture_type == ITexture::Type::DepthStencilBuffer)
                wait_stages |= vk::PipelineStageFlagBits::eVertexShader;
        }
    }
    return wait_stages;
}

Ptr<CommandList::DebugGroup> CommandList::DebugGroup::Create(const std::string& name)
{
    META_FUNCTION_TASK();
    return std::make_shared<ICommandListVK::DebugGroupVK>(name);
}

ICommandListVK::DebugGroupVK::DebugGroupVK(const std::string& name)
    : CommandListBase::DebugGroupBase(name)
    , m_vk_debug_label(ObjectBase::GetName().c_str())
{
    META_FUNCTION_TASK();
}

Ptr<CommandListSet> CommandListSet::Create(const Refs<CommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListSetVK>(command_list_refs, frame_index_opt);
}

CommandListSetVK::CommandListSetVK(const Refs<CommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : CommandListSetBase(command_list_refs, frame_index_opt)
    , m_vk_wait_frame_buffer_rendering_on_stages(GetFrameBufferRenderingWaitStages(command_list_refs))
    , m_vk_device(GetCommandQueueVK().GetContextVK().GetDeviceVK().GetNativeDevice())
    , m_vk_unique_execution_completed_semaphore(m_vk_device.createSemaphoreUnique(vk::SemaphoreCreateInfo()))
    , m_vk_unique_execution_completed_fence(m_vk_device.createFenceUnique(vk::FenceCreateInfo()))
{
    META_FUNCTION_TASK();
    const Refs<CommandListBase>& base_command_list_refs = GetBaseRefs();
    
    m_vk_command_buffers.reserve(command_list_refs.size());
    for (const Ref<CommandListBase>& command_list_ref : base_command_list_refs)
    {
        const CommandListBase& command_list = command_list_ref.get();
        const auto& vulkan_command_list = command_list.GetType() == CommandList::Type::ParallelRender
                                        ? static_cast<const ParallelRenderCommandListVK&>(command_list).GetPrimaryCommandListVK()
                                        : dynamic_cast<const ICommandListVK&>(command_list_ref.get());
        m_vk_command_buffers.emplace_back(vulkan_command_list.GetNativeCommandBuffer());
    }

    UpdateNativeDebugName();
}

void CommandListSetVK::Execute(const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandListSetBase::Execute(completed_callback);

    vk::SubmitInfo submit_info(
        GetWaitSemaphores(),
        GetWaitStages(),
        m_vk_command_buffers,
        GetNativeExecutionCompletedSemaphore()
    );
    
    Opt<vk::TimelineSemaphoreSubmitInfo> vk_timeline_submit_info_opt;
    if (const std::vector<uint64_t>& vk_wait_values = CommandListSetVK::GetWaitValues();
        !vk_wait_values.empty())
    {
        META_CHECK_ARG_EQUAL(vk_wait_values.size(), submit_info.waitSemaphoreCount);
        vk_timeline_submit_info_opt.emplace(vk_wait_values);
        submit_info.setPNext(&vk_timeline_submit_info_opt.value());
    }

    std::scoped_lock fence_guard(m_vk_unique_execution_completed_fence_mutex);
    m_vk_device.resetFences(GetNativeExecutionCompletedFence());
    GetCommandQueueVK().GetNativeQueue().submit(submit_info, GetNativeExecutionCompletedFence());
}

void CommandListSetVK::WaitUntilCompleted()
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

CommandQueueVK& CommandListSetVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(GetCommandQueueBase());
}

const CommandQueueVK& CommandListSetVK::GetCommandQueueVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const CommandQueueVK&>(GetCommandQueueBase());
}

const std::vector<vk::Semaphore>& CommandListSetVK::GetWaitSemaphores()
{
    META_FUNCTION_TASK();
    const CommandQueueVK& command_queue = GetCommandQueueVK();
    const std::vector<vk::Semaphore>& vk_wait_semaphores = command_queue.GetWaitBeforeExecuting().semaphores;
    if (!m_vk_wait_frame_buffer_rendering_on_stages)
        return vk_wait_semaphores;

    m_vk_wait_semaphores = vk_wait_semaphores;
    m_vk_wait_semaphores.push_back(dynamic_cast<const RenderContextVK&>(command_queue.GetContextVK()).GetNativeFrameImageAvailableSemaphore());
    return m_vk_wait_semaphores;
}

const std::vector<vk::PipelineStageFlags>& CommandListSetVK::GetWaitStages()
{
    META_FUNCTION_TASK();
    const CommandQueueVK& command_queue = GetCommandQueueVK();
    const std::vector<vk::PipelineStageFlags>& vk_wait_stages = command_queue.GetWaitBeforeExecuting().stages;
    if (!m_vk_wait_frame_buffer_rendering_on_stages)
        return vk_wait_stages;

    m_vk_wait_stages = vk_wait_stages;
    m_vk_wait_stages.push_back(m_vk_wait_frame_buffer_rendering_on_stages);
    return m_vk_wait_stages;
}

const std::vector<uint64_t>& CommandListSetVK::GetWaitValues()
{
    META_FUNCTION_TASK();
    const CommandQueueVK& command_queue = GetCommandQueueVK();
    const CommandQueueVK::WaitInfo& wait_before_exec = command_queue.GetWaitBeforeExecuting();
    META_CHECK_ARG_EQUAL(wait_before_exec.wait_values.size(), wait_before_exec.semaphores.size());

    const std::vector<uint64_t>& vk_wait_values = wait_before_exec.wait_values;
    if (!m_vk_wait_frame_buffer_rendering_on_stages || vk_wait_values.empty())
        return vk_wait_values;

    m_vk_wait_values = vk_wait_values;
    m_vk_wait_values.push_back(0U);
    return m_vk_wait_values;
}

void CommandListSetVK::OnObjectNameChanged(IObject& object, const std::string& old_name)
{
    META_FUNCTION_TASK();
    CommandListSetBase::OnObjectNameChanged(object, old_name);
    UpdateNativeDebugName();
}

void CommandListSetVK::UpdateNativeDebugName()
{
    META_FUNCTION_TASK();
    const std::string execution_completed_name = fmt::format("{} Execution Completed", GetCombinedName());
    SetVulkanObjectName(m_vk_device, m_vk_unique_execution_completed_semaphore.get(), execution_completed_name);
    SetVulkanObjectName(m_vk_device, m_vk_unique_execution_completed_fence.get(), execution_completed_name);
}

} // namespace Methane::Graphics
