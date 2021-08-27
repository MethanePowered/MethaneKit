/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/CommandListVK.hpp
Vulkan base template implementation of the command list interface.

******************************************************************************/

#pragma once

#include "CommandListVK.h"
#include "CommandQueueVK.h"
#include "DeviceVK.h"
#include "ContextVK.h"
#include "ResourceVK.hpp"
#include "ProgramBindingsVK.h"

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <vulkan/vulkan.hpp>
#include <nowide/convert.hpp>
#include <fmt/format.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

template<class CommandListBaseT, typename = std::enable_if_t<std::is_base_of_v<CommandListBase, CommandListBaseT>>>
class CommandListVK
    : public CommandListBaseT
    , public ICommandListVK
{
public:
    template<typename... ConstructArgs>
    explicit CommandListVK(ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
        , m_vk_command_buffer(GetCommandQueueVK().GetContextVK().GetDeviceVK().GetNativeDevice().allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(
                GetCommandQueueVK().GetNativeCommandPool(),
                vk::CommandBufferLevel::ePrimary,
                1U
            )).back())
    {
        META_FUNCTION_TASK();

        m_vk_command_buffer.begin(vk::CommandBufferBeginInfo());
        CommandListBaseT::SetCommandListState(CommandList::State::Encoding);
    }

    ~CommandListVK()
    {
        META_FUNCTION_TASK();
        GetCommandQueueVK().GetContextVK().GetDeviceVK().GetNativeDevice().freeCommandBuffers(GetCommandQueueVK().GetNativeCommandPool(), m_vk_command_buffer);
    }

    // CommandList interface

    void PushDebugGroup(CommandList::DebugGroup& debug_group) final
    {
        META_FUNCTION_TASK();
        CommandListBase::PushDebugGroup(debug_group);
        // TODO: add support of Vulkan debug groups
    }

    void PopDebugGroup() final
    {
        META_FUNCTION_TASK();
        CommandListBase::PopDebugGroup();
        // TODO: add support of Vulkan debug groups
    }

    void Commit() override
    {
        META_FUNCTION_TASK();
        CommandListBaseT::Commit();

        // TODO: insert ending timestamp query

        m_vk_command_buffer.end();
        m_is_native_committed = true;
    }

    void SetResourceBarriers(const Resource::Barriers& resource_barriers) final
    {
        META_FUNCTION_TASK();
        CommandListBaseT::VerifyEncodingState();
        
        const auto lock_guard = resource_barriers.Lock();
        if (resource_barriers.IsEmpty())
            return;

        META_LOG("{} Command list '{}' SET RESOURCE BARRIERS:\n{}", magic_enum::enum_name(GetType()), GetName(), static_cast<std::string>(resource_barriers));

        // TODO: set Vulkan resource barriers
    }

    // CommandList interface

    void Reset(CommandList::DebugGroup* p_debug_group) override
    {
        META_FUNCTION_TASK();
        if (!m_is_native_committed)
            return;

        m_is_native_committed = false;
        m_vk_command_buffer.begin(vk::CommandBufferBeginInfo());

        // TODO: insert beginning timestamp query

        CommandListBase::Reset(p_debug_group);
    }

    Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const final
    {
        META_FUNCTION_TASK();
        // TODO: add support for timestamps query
        return CommandListBase::GetGpuTimeRange(in_cpu_nanoseconds);
    }

    // Object interface

    void SetName(const std::string& name) final
    {
        META_FUNCTION_TASK();
        // TODO: set command buffer debug name
        CommandListBaseT::SetName(name);
    }

    // ICommandListVK interface
    CommandQueueVK&          GetCommandQueueVK() final            { return static_cast<CommandQueueVK&>(CommandListBaseT::GetCommandQueueBase()); }
    const CommandQueueVK&    GetCommandQueueVK() const final      { return static_cast<const CommandQueueVK&>(CommandListBaseT::GetCommandQueueBase()); }
    const vk::CommandBuffer& GetNativeCommandBuffer() const final { return m_vk_command_buffer; }

protected:
    void ApplyProgramBindings(ProgramBindingsBase& program_bindings, ProgramBindings::ApplyBehavior apply_behavior) final
    {
        // Optimization to skip dynamic_cast required to call Apply method of the ProgramBindingBase implementation
        static_cast<ProgramBindingsVK&>(program_bindings).Apply(*this, CommandListBase::GetProgramBindings().get(), apply_behavior);
    }

    bool IsNativeCommitted() const             { return m_is_native_committed; }
    void SetNativeCommitted(bool is_committed) { m_is_native_committed = is_committed; }

private:
    vk::CommandBuffer m_vk_command_buffer;
    bool              m_is_native_committed = false;
};

} // namespace Methane::Graphics
