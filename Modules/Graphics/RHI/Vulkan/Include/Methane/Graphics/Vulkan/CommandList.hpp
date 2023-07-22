/******************************************************************************

Copyright 2021-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/CommandList.hpp
Vulkan base template implementation of the command list interface.

******************************************************************************/

#pragma once

#include "ICommandList.h"
#include "CommandListDebugGroup.h"
#include "CommandQueue.h"
#include "Device.h"
#include "IContext.h"
#include "ProgramBindings.h"
#include "ResourceBarriers.h"
#include "Utils.hpp"

#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Graphics/Base/ResourceBarriers.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <vulkan/vulkan.hpp>
#include <nowide/convert.hpp>
#include <fmt/format.h>
#include <magic_enum.hpp>

#include <array>
#include <vector>

namespace Methane::Graphics::Vulkan
{

class ParallelRenderCommandList;

template<class CommandListBaseT, vk::PipelineBindPoint pipeline_bind_point, uint32_t command_buffers_count = 1U,
         CommandBufferType default_command_buffer_type = CommandBufferType::Primary,
         typename = std::enable_if_t<std::is_base_of_v<Base::CommandList, CommandListBaseT> && command_buffers_count != 0U>>
class CommandList
    : public CommandListBaseT
    , public ICommandList
{
public:
    template<typename... ConstructArgs, uint32_t buffers_count = command_buffers_count,
             typename = std::enable_if_t< buffers_count != 1>>
    CommandList(const vk::CommandBufferInheritanceInfo& secondary_render_buffer_inherit_info, ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
        , m_vk_device(GetVulkanCommandQueue().GetVulkanContext().GetVulkanDevice().GetNativeDevice()) // NOSONAR
        , m_vk_unique_command_pool(CreateVulkanCommandPool(GetVulkanCommandQueue().GetFamilyIndex()))
    {
        META_FUNCTION_TASK();
        std::fill(m_vk_command_buffer_encoding_flags.begin(), m_vk_command_buffer_encoding_flags.end(), false);
        std::fill(m_vk_command_buffer_primary_flags.begin(), m_vk_command_buffer_primary_flags.end(), false);

        InitializePrimaryCommandBuffer();
        SetSecondaryRenderBufferInheritInfo(secondary_render_buffer_inherit_info);
        InitializeSecondaryCommandBuffers(1U);

        CommandListBaseT::InitializeTimestampQueries();
        CommandListBaseT::BeginGpuZone();

        CommandListBaseT::SetCommandListState(Rhi::CommandListState::Encoding);
    }

    CommandList(const vk::CommandBufferInheritanceInfo& secondary_render_buffer_inherit_info, ParallelRenderCommandList& parallel_render_command_list, bool is_beginning_cmd_list)
        : CommandListBaseT(parallel_render_command_list)
        , m_vk_device(GetVulkanCommandQueue().GetVulkanContext().GetVulkanDevice().GetNativeDevice()) // NOSONAR
        , m_vk_unique_command_pool(CreateVulkanCommandPool(GetVulkanCommandQueue().GetFamilyIndex()))
        , m_debug_group_command_buffer_type(is_beginning_cmd_list ? CommandBufferType::Primary : default_command_buffer_type)
    {
        META_FUNCTION_TASK();
        std::fill(m_vk_command_buffer_encoding_flags.begin(), m_vk_command_buffer_encoding_flags.end(), false);
        std::fill(m_vk_command_buffer_primary_flags.begin(), m_vk_command_buffer_primary_flags.end(), false);

        if (is_beginning_cmd_list)
        {
            // Beginning command list of the parallel rendering requires only primary command buffer for submitting all other commands
            InitializePrimaryCommandBuffer();

            // Timestamp queries are used only in the beginning command list with Primary command buffer
            // because queries can not be performed inside render pass, but thread render command lists have only render pass commands
            CommandListBaseT::InitializeTimestampQueries();
            CommandListBaseT::BeginGpuZone();
        }
        else
        {
            // Thread render and ending command lists of the parallel rendering do not use primary command buffers
            SetSecondaryRenderBufferInheritInfo(secondary_render_buffer_inherit_info);
            InitializeSecondaryCommandBuffers(0U);
        }

        CommandListBaseT::SetCommandListState(Rhi::CommandListState::Encoding);
    }

    template<typename... ConstructArgs, uint32_t buffers_count = command_buffers_count,
             typename = std::enable_if_t<buffers_count == 1>>
    CommandList(const vk::CommandBufferLevel& vk_buffer_level, const vk::CommandBufferBeginInfo& vk_begin_info, ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
        , m_vk_device(GetVulkanCommandQueue().GetVulkanContext().GetVulkanDevice().GetNativeDevice()) // NOSONAR
        , m_vk_unique_command_pool(CreateVulkanCommandPool(GetVulkanCommandQueue().GetFamilyIndex()))
        , m_vk_command_buffer_begin_infos({ vk_begin_info })
    {
        META_FUNCTION_TASK();
        std::fill(m_vk_command_buffer_encoding_flags.begin(), m_vk_command_buffer_encoding_flags.end(), false);
        std::fill(m_vk_command_buffer_primary_flags.begin(), m_vk_command_buffer_primary_flags.end(), false);

        InitializePrimaryCommandBuffer(vk_buffer_level);

        if (vk_buffer_level == vk::CommandBufferLevel::ePrimary)
        {
            CommandListBaseT::InitializeTimestampQueries();
            CommandListBaseT::BeginGpuZone();
        }

        CommandListBaseT::SetCommandListState(Rhi::CommandListState::Encoding);
    }

    // ICommandList interface

    void PushDebugGroup(Rhi::ICommandListDebugGroup& debug_group) final
    {
        META_FUNCTION_TASK();
        Base::CommandList::PushDebugGroup(debug_group);
        GetNativeCommandBuffer(m_debug_group_command_buffer_type).beginDebugUtilsLabelEXT(static_cast<const CommandListDebugGroup&>(debug_group).GetNativeDebugLabel());
    }

    void PopDebugGroup() final
    {
        META_FUNCTION_TASK();
        Base::CommandList::PopDebugGroup();
        GetNativeCommandBuffer(m_debug_group_command_buffer_type).endDebugUtilsLabelEXT();
    }

    void Commit() override
    {
        META_FUNCTION_TASK();
        const auto state_lock = Base::CommandList::LockStateMutex();

        CommandListBaseT::Commit();
        CommandListBaseT::EndGpuZone();

        // End command buffers encoding
        for (size_t cmd_buffer_index = 0; cmd_buffer_index < command_buffers_count; ++cmd_buffer_index)
        {
            if (!m_vk_command_buffer_encoding_flags[cmd_buffer_index])
                continue;

            m_vk_unique_command_buffers[cmd_buffer_index].get().end();
            m_vk_command_buffer_encoding_flags[cmd_buffer_index] = false;
        }

        m_is_native_committed = true;
    }

    void SetResourceBarriers(const Rhi::IResourceBarriers& resource_barriers) final
    {
        META_FUNCTION_TASK();
        CommandListBaseT::VerifyEncodingState();

        const auto lock_guard = static_cast<const Base::ResourceBarriers&>(resource_barriers).Lock();
        if (resource_barriers.IsEmpty())
            return;

        META_LOG("{} Command list '{}' SET RESOURCE BARRIERS:\n{}",
            magic_enum::enum_name(Base::CommandList::GetType()),
            Base::CommandList::GetName(),
            static_cast<std::string>(resource_barriers));

        const auto& vulkan_resource_barriers = static_cast<const ResourceBarriers&>(resource_barriers);
        const ResourceBarriers::NativePipelineBarrier& pipeline_barrier = vulkan_resource_barriers.GetNativePipelineBarrierData(GetVulkanCommandQueue());

        GetNativeCommandBuffer(CommandBufferType::Primary).pipelineBarrier(
            pipeline_barrier.vk_src_stage_mask,
            pipeline_barrier.vk_dst_stage_mask,
            vk::DependencyFlags{},
            pipeline_barrier.vk_memory_barriers,
            pipeline_barrier.vk_buffer_memory_barriers,
            pipeline_barrier.vk_image_memory_barriers
        );
    }

    // ICommandList interface

    void Reset(Rhi::ICommandListDebugGroup* debug_group_ptr) override
    {
        META_FUNCTION_TASK();
        const auto state_lock = Base::CommandList::LockStateMutex();
        if (!m_is_native_committed)
            return;

        m_is_native_committed = false;

        // Begin command buffers encoding
        for (size_t cmd_buffer_index = 0; cmd_buffer_index < command_buffers_count; ++cmd_buffer_index)
        {
            if (m_vk_command_buffer_encoding_flags[cmd_buffer_index] || !m_vk_unique_command_buffers[cmd_buffer_index])
                continue;

            m_vk_unique_command_buffers[cmd_buffer_index].get().begin(GetCommandBufferBeginInfo(static_cast<CommandBufferType>(cmd_buffer_index)));
            m_vk_command_buffer_encoding_flags[cmd_buffer_index] = true;
        }

        CommandListBaseT::BeginGpuZone();

        Base::CommandList::Reset(debug_group_ptr);
    }

    // IObject interface

    bool SetName(std::string_view name) final
    {
        META_FUNCTION_TASK();
        if (!CommandListBaseT::SetName(name))
            return false;

        SetVulkanObjectName(m_vk_device, m_vk_unique_command_pool.get(), fmt::format("{} Command Pool", name));
        for (size_t cmd_buffer_index = 0; cmd_buffer_index < command_buffers_count; ++cmd_buffer_index)
        {
            const vk::UniqueCommandBuffer& vk_unique_command_buffer = m_vk_unique_command_buffers[cmd_buffer_index];
            if (!vk_unique_command_buffer)
                continue;

            SetVulkanObjectName(m_vk_device, vk_unique_command_buffer.get(),
                                fmt::format("{} ({})", name.data(), magic_enum::enum_name(static_cast<CommandBufferType>(cmd_buffer_index))));
        }
        return true;
    }

    // ICommandList interface
    CommandQueue&            GetVulkanCommandQueue() final               { return static_cast<CommandQueue&>(CommandListBaseT::GetBaseCommandQueue()); }
    const CommandQueue&      GetVulkanCommandQueue() const final         { return static_cast<const CommandQueue&>(CommandListBaseT::GetBaseCommandQueue()); }
    vk::PipelineBindPoint    GetNativePipelineBindPoint() const final    { return pipeline_bind_point; }
    const vk::CommandBuffer& GetNativeCommandBufferDefault() const final { return GetNativeCommandBuffer(default_command_buffer_type); }
    const vk::CommandBuffer& GetNativeCommandBuffer(CommandBufferType cmd_buffer_type) const final
    {
        META_FUNCTION_TASK();
        const size_t cmd_buffer_index = magic_enum::enum_index(cmd_buffer_type).value();
        META_CHECK_ARG_LESS_DESCR(cmd_buffer_index, command_buffers_count, "Not enough command buffers count for {}",
                                  magic_enum::enum_name(cmd_buffer_type));
        return m_vk_unique_command_buffers[cmd_buffer_index].get();
    }
    

protected:
    bool IsNativeCommitted() const             { return m_is_native_committed; }
    void SetNativeCommitted(bool is_committed) { m_is_native_committed = is_committed; }

    void CommitCommandBuffer(CommandBufferType cmd_buffer_type)
    {
        META_FUNCTION_TASK();
        const auto cmd_buffer_index = static_cast<uint32_t>(cmd_buffer_type);
        if (!m_vk_command_buffer_encoding_flags[cmd_buffer_index])
            return;

        m_vk_unique_command_buffers[cmd_buffer_index].get().end();
        m_vk_command_buffer_encoding_flags[cmd_buffer_index] = false;
    }

    void ApplyProgramBindings(Base::ProgramBindings& program_bindings, Rhi::ProgramBindingsApplyBehaviorMask apply_behavior) final
    {
        // Optimization to skip dynamic_cast required to call Apply method of the Base::ProgramBinding implementation
        static_cast<ProgramBindings&>(program_bindings).Apply(*this, Base::CommandList::GetCommandQueue(),
                                                                Base::CommandList::GetProgramBindingsPtr(), apply_behavior);
    }

    void SetSecondaryRenderBufferInheritInfo(const vk::CommandBufferInheritanceInfo& secondary_render_buffer_inherit_info) noexcept
    {
        META_FUNCTION_TASK();
        m_vk_secondary_render_buffer_inherit_info_opt = secondary_render_buffer_inherit_info;
        const size_t secondary_render_pass_index = magic_enum::enum_index(CommandBufferType::SecondaryRenderPass).value();
        const bool is_secondary_command_buffer = !m_vk_command_buffer_primary_flags[secondary_render_pass_index];
        m_vk_command_buffer_begin_infos[secondary_render_pass_index] = vk::CommandBufferBeginInfo(
            is_secondary_command_buffer && secondary_render_buffer_inherit_info.renderPass
                ? vk::CommandBufferUsageFlags(vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
                : vk::CommandBufferUsageFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit),
            &m_vk_secondary_render_buffer_inherit_info_opt.value()
        );
    }

    vk::UniqueCommandPool CreateVulkanCommandPool(uint32_t queue_family_index)
    {
        META_FUNCTION_TASK();
        vk::CommandPoolCreateInfo vk_command_pool_info(vk::CommandPoolCreateFlags(), queue_family_index);
        vk_command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        return m_vk_device.createCommandPoolUnique(vk_command_pool_info);
    }

    void InitializePrimaryCommandBuffer(const vk::CommandBufferLevel& vk_buffer_level = vk::CommandBufferLevel::ePrimary)
    {
        META_FUNCTION_TASK();
        m_vk_command_buffer_primary_flags[0] = true;
        m_vk_unique_command_buffers[0] = std::move(m_vk_device.allocateCommandBuffersUnique(
            vk::CommandBufferAllocateInfo(m_vk_unique_command_pool.get(), vk_buffer_level, 1U)
        ).back());

        m_vk_unique_command_buffers[0].get().begin(m_vk_command_buffer_begin_infos[0]);
        m_vk_command_buffer_encoding_flags[0] = true;
    }

    void InitializeSecondaryCommandBuffers(uint32_t offset_primary_count)
    {
        META_FUNCTION_TASK();
        std::vector<vk::UniqueCommandBuffer> secondary_cmd_buffers = m_vk_device.allocateCommandBuffersUnique(
            vk::CommandBufferAllocateInfo(
                m_vk_unique_command_pool.get(),
                vk::CommandBufferLevel::eSecondary,
                command_buffers_count - offset_primary_count
            ));

        for (uint32_t cmd_buffer_index = offset_primary_count; cmd_buffer_index < command_buffers_count; ++cmd_buffer_index)
        {
            vk::UniqueCommandBuffer& vk_unique_command_buffer = m_vk_unique_command_buffers[cmd_buffer_index];
            vk_unique_command_buffer = std::move(secondary_cmd_buffers[cmd_buffer_index - offset_primary_count]);

            const vk::CommandBufferBeginInfo& secondary_begin_info = GetCommandBufferBeginInfo(static_cast<CommandBufferType>(cmd_buffer_index));
            if (!secondary_begin_info.pInheritanceInfo)
            {
                static const vk::CommandBufferInheritanceInfo s_default_secondary_inheritance_info;
                m_vk_command_buffer_begin_infos[cmd_buffer_index].pInheritanceInfo = &s_default_secondary_inheritance_info;
            }

            vk_unique_command_buffer.get().begin(secondary_begin_info);
            m_vk_command_buffer_encoding_flags[cmd_buffer_index] = true;
        }
    }

    const vk::CommandBufferBeginInfo& GetCommandBufferBeginInfo(CommandBufferType cmd_buffer_type) const
    {
        META_FUNCTION_TASK();
        return m_vk_command_buffer_begin_infos[magic_enum::enum_index(cmd_buffer_type).value()];       
    }

private:
    vk::Device            m_vk_device;
    vk::UniqueCommandPool m_vk_unique_command_pool;
    bool                  m_is_native_committed = false;

    // Unique command buffers and corresponding begin flags are indexed by the value of CommandBufferType enum
    std::array<vk::UniqueCommandBuffer, command_buffers_count>    m_vk_unique_command_buffers;
    std::array<bool, command_buffers_count>                       m_vk_command_buffer_primary_flags;
    std::array<bool, command_buffers_count>                       m_vk_command_buffer_encoding_flags;
    std::array<vk::CommandBufferBeginInfo, command_buffers_count> m_vk_command_buffer_begin_infos;
    std::optional<vk::CommandBufferInheritanceInfo> m_vk_secondary_render_buffer_inherit_info_opt;
    const CommandBufferType           m_debug_group_command_buffer_type = default_command_buffer_type;
};

} // namespace Methane::Graphics::Vulkan
