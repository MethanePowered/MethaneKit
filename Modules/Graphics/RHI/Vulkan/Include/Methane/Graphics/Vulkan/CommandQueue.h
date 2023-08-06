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

FILE: Methane/Graphics/Vulkan/CommandQueue.h
Vulkan implementation of the command queue interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandQueueTracking.h>
#include <Methane/Instrumentation.h>

#include <vulkan/vulkan.hpp>
#include <mutex>

namespace Methane::Graphics::Vulkan
{

class Device;
class RenderPass;
class QueueFamilyReservation;
struct IContext;

class CommandQueue final // NOSONAR - custom destructor is required
    : public Base::CommandQueueTracking
{
public:
    struct WaitInfo
    {
        std::vector<vk::Semaphore>          semaphores;
        std::vector<vk::PipelineStageFlags> stages;
        std::vector<uint64_t>               values;
    };

    CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type);
    ~CommandQueue() override;

    // ICommandQueue interface
    [[nodiscard]] Ptr<Rhi::IFence>                     CreateFence() override;
    [[nodiscard]] Ptr<Rhi::ITransferCommandList>       CreateTransferCommandList() override;
    [[nodiscard]] Ptr<Rhi::IComputeCommandList>        CreateComputeCommandList() override;
    [[nodiscard]] Ptr<Rhi::IRenderCommandList>         CreateRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::IParallelRenderCommandList> CreateParallelRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::ITimestampQueryPool>        CreateTimestampQueryPool(uint32_t max_timestamps_per_frame) override;
    uint32_t GetFamilyIndex() const noexcept override { return m_queue_family_index; }
    void Execute(Rhi::ICommandListSet& command_list_set, const Rhi::ICommandList::CompletedCallback& completed_callback = {}) override;

    // IObject interface
    bool SetName(std::string_view name) override;

    const IContext& GetVulkanContext() const noexcept { return m_vk_context; }
    Device& GetVulkanDevice() const noexcept;

    void WaitForSemaphore(const vk::Semaphore& semaphore, vk::PipelineStageFlags stage_flags, const uint64_t* timeline_wait_value_ptr = nullptr);
    const WaitInfo& GetWaitBeforeExecuting() const noexcept { return m_wait_before_executing; }
    const WaitInfo& GetWaitForExecutionCompleted() const;
    const WaitInfo& GetWaitForFrameExecutionCompleted(Data::Index frame_index) const;
    void ResetWaitForFrameExecution(Data::Index frame_index);

    uint32_t GetNativeQueueFamilyIndex() const noexcept { return m_queue_family_index; }
    uint32_t GetNativeQueueIndex() const noexcept       { return m_queue_index; }

    vk::Queue&       GetNativeQueue() noexcept          { return m_vk_queue; }
    const vk::Queue& GetNativeQueue() const noexcept    { return m_vk_queue; }

    vk::PipelineStageFlags GetNativeSupportedStageFlags() const noexcept    { return m_vk_supported_stage_flags; }
    vk::AccessFlags        GetNativeSupportedAccessFlags() const noexcept   { return m_vk_supported_access_flags; }

protected:
    // Base::CommandQueueTracking override
    void CompleteCommandListSetExecution(Base::CommandListSet& executing_command_list_set) override;

private:
    CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type, const Device& device);
    CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type, const Device& device,
                 const QueueFamilyReservation& family_reservation);
    CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type, const Device& device,
                 const QueueFamilyReservation& family_reservation, const vk::QueueFamilyProperties& family_properties);

    void Reset();
    void AddWaitForFrameExecution(const Rhi::ICommandListSet& command_list_set);

    using FrameWaitInfos = std::vector<WaitInfo>;

    const IContext&        m_vk_context;
    const uint32_t         m_queue_family_index;
    const uint32_t         m_queue_index;
    vk::Queue              m_vk_queue;
    vk::PipelineStageFlags m_vk_supported_stage_flags;
    vk::AccessFlags        m_vk_supported_access_flags;
    WaitInfo               m_wait_before_executing;
    mutable WaitInfo       m_wait_execution_completed;
    FrameWaitInfos         m_wait_frame_execution_completed;
    mutable TracyLockable(std::mutex, m_wait_frame_execution_completed_mutex);
};

} // namespace Methane::Graphics::Vulkan
