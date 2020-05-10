/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/CommandListMT.hpp
Metal base template implementation of the command list interface.

******************************************************************************/

#pragma once

#include "CommandQueueMT.hh"
#include "CommandListMT.hh"

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

#import <Metal/Metal.h>

#include <stdexcept>

namespace Methane::Graphics
{

template<typename MTLCommandEncoderId, class CommandListBaseT, typename = std::enable_if_t<std::is_base_of_v<CommandListBase, CommandListBaseT>>>
class CommandListMT
    : public CommandListBaseT
{
public:
    template<typename... ConstructArgs>
    explicit CommandListMT(bool is_command_buffer_enabled, ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
        , m_is_cmd_buffer_enabled(is_command_buffer_enabled)
    {
        META_FUNCTION_TASK();
    }

    // CommandList interface
    void PushDebugGroup(CommandList::DebugGroup& debug_group) override
    {
        META_FUNCTION_TASK();

        CommandListBaseT::PushDebugGroup(debug_group);

        assert(m_mtl_cmd_encoder != nil);
        [m_mtl_cmd_encoder pushDebugGroup:static_cast<CommandListDebugGroupMT&>(debug_group).GetNSName()];
    }

    void PopDebugGroup() override
    {
        META_FUNCTION_TASK();

        CommandListBaseT::PopDebugGroup();

        assert(m_mtl_cmd_encoder != nil);
        [m_mtl_cmd_encoder popDebugGroup];
    }

    void Commit() override
    {
        META_FUNCTION_TASK();

        assert(!CommandListBaseT::IsCommitted());
        CommandListBaseT::Commit();

        if (m_mtl_cmd_encoder)
        {
            [m_mtl_cmd_encoder endEncoding];
            m_mtl_cmd_encoder = nil;
        }

        if (!m_is_cmd_buffer_enabled || !m_mtl_cmd_buffer)
            return;

        [m_mtl_cmd_buffer enqueue];
    }

    Data::TimeRange GetGpuTimeRange() const override
    {
        META_FUNCTION_TASK();
        if (CommandListBaseT::GetState() != CommandListBase::State::Pending)
            throw std::logic_error("Can not get GPU time range of executing or not committed command list.");

        if (!m_mtl_cmd_buffer)
            return Data::TimeRange();

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
        if (@available(macOS 10.15, *))
        {
            assert(m_mtl_cmd_buffer.status == MTLCommandBufferStatusCompleted);
            return Data::TimeRange(
                Data::ConvertTimeSecondsToNanoseconds(m_mtl_cmd_buffer.GPUStartTime),
                Data::ConvertTimeSecondsToNanoseconds(m_mtl_cmd_buffer.GPUEndTime)
            );
        }
#endif
        return Data::TimeRange();
    }

    // CommandListBase interface

    void SetResourceBarriers(const ResourceBase::Barriers&) override { }

    void Execute(uint32_t frame_index, const CommandList::CompletedCallback& completed_callback) override
    {
        META_FUNCTION_TASK();

        CommandListBaseT::Execute(frame_index, completed_callback);

        if (!m_is_cmd_buffer_enabled || !m_mtl_cmd_buffer)
            return;

        [m_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
            CommandListBaseT::Complete(frame_index);
            m_mtl_cmd_buffer  = nil;
        }];

        [m_mtl_cmd_buffer commit];
    }

    // Object interface

    void SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();

        CommandListBaseT::SetName(name);
        m_ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);

        if (m_mtl_cmd_encoder != nil)
        {
            m_mtl_cmd_encoder.label = m_ns_name;
        }

        if (m_mtl_cmd_buffer != nil)
        {
            m_mtl_cmd_buffer.label = m_ns_name;
        }
    }

    MTLCommandEncoderId& GetNativeCommandEncoder() noexcept { return m_mtl_cmd_encoder; }

    CommandQueueMT& GetCommandQueueMT() noexcept
    {
        META_FUNCTION_TASK();
        return static_cast<class CommandQueueMT&>(CommandListBaseT::GetCommandQueue());
    }

protected:
    id<MTLCommandBuffer>& InitializeCommandBuffer()
    {
        META_FUNCTION_TASK();
        if (m_mtl_cmd_buffer)
            return m_mtl_cmd_buffer;

        m_mtl_cmd_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];
        m_mtl_cmd_buffer.label = m_ns_name;
        return m_mtl_cmd_buffer;
    }

    void InitializeCommandEncoder(MTLCommandEncoderId&& mtl_cmd_encoder)
    {
        META_FUNCTION_TASK();
        m_mtl_cmd_encoder = std::move(mtl_cmd_encoder);
        m_mtl_cmd_encoder.label = m_ns_name;
    }

    bool IsCommandBufferInitialized() const noexcept   { return m_mtl_cmd_buffer; }
    bool IsCommandEncoderInitialized() const noexcept  { return m_mtl_cmd_encoder; }

private:
    const bool           m_is_cmd_buffer_enabled;
    id<MTLCommandBuffer> m_mtl_cmd_buffer = nil;
    MTLCommandEncoderId  m_mtl_cmd_encoder = nil;
    NSString*            m_ns_name = nil;
};

} // namespace Methane::Graphics
