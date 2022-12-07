/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/CommandList.hpp
Metal base template implementation of the command list interface.

******************************************************************************/

#pragma once

#include "CommandQueue.hh"
#include "CommandList.hh"

#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#import <Metal/Metal.h>

#include <stdexcept>

namespace Methane::Graphics::Metal
{

template<typename MTLCommandEncoderId, class CommandListBaseT, typename = std::enable_if_t<std::is_base_of_v<Base::CommandList, CommandListBaseT>>>
class CommandList : public CommandListBaseT
{
public:
    template<typename... ConstructArgs>
    explicit CommandList(bool is_command_buffer_enabled, ConstructArgs&&... construct_args)
        : CommandListBaseT(std::forward<ConstructArgs>(construct_args)...)
        , m_is_cmd_buffer_enabled(is_command_buffer_enabled)
    {
        META_FUNCTION_TASK();
    }

    // ICommandList interface
    void PushDebugGroup(Rhi::ICommandListDebugGroup& debug_group) override
    {
        META_FUNCTION_TASK();
        std::scoped_lock lock_guard(m_cmd_buffer_mutex);

        CommandListBaseT::PushDebugGroup(debug_group);

        META_CHECK_ARG_NOT_NULL(m_mtl_cmd_encoder);
        [m_mtl_cmd_encoder pushDebugGroup:static_cast<CommandListDebugGroup&>(debug_group).GetNSName()];
    }

    void PopDebugGroup() override
    {
        META_FUNCTION_TASK();
        std::scoped_lock lock_guard(m_cmd_buffer_mutex);

        CommandListBaseT::PopDebugGroup();

        META_CHECK_ARG_NOT_NULL(m_mtl_cmd_encoder);
        [m_mtl_cmd_encoder popDebugGroup];
    }

    void Commit() override
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_FALSE(CommandListBaseT::IsCommitted());

        CommandListBaseT::Commit();

        std::scoped_lock lock_guard(m_cmd_buffer_mutex);

        if (m_mtl_cmd_encoder)
        {
            [m_mtl_cmd_encoder endEncoding];
            m_mtl_cmd_encoder = nil;
        }

        if (!m_is_cmd_buffer_enabled || m_mtl_cmd_buffer != nil)
            return;

        [m_mtl_cmd_buffer enqueue];
    }

    Data::TimeRange GetGpuTimeRange(bool in_cpu_nanoseconds) const final
    {
        META_FUNCTION_TASK();
        META_UNUSED(in_cpu_nanoseconds);
        META_CHECK_ARG_EQUAL_DESCR(CommandListBaseT::GetState(), Base::CommandList::State::Pending,
                                   "can not get GPU time range of executing or not committed command list");

        if (!m_mtl_cmd_buffer)
            return Data::TimeRange();

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
        if (@available(macOS 10.15, *))
        {
            META_CHECK_ARG_EQUAL(m_mtl_cmd_buffer.status, MTLCommandBufferStatusCompleted);
            return Data::TimeRange(
                Data::ConvertTimeSecondsToNanoseconds(m_mtl_cmd_buffer.GPUStartTime),
                Data::ConvertTimeSecondsToNanoseconds(m_mtl_cmd_buffer.GPUEndTime)
            );
        }
#endif
        return Data::TimeRange();
    }

    // Base::CommandList interface

    void SetResourceBarriers(const Rhi::IResourceBarriers&) override { }

    void Execute(const Rhi::ICommandList::CompletedCallback& completed_callback) override
    {
        META_FUNCTION_TASK();
        std::scoped_lock lock_guard(m_cmd_buffer_mutex);

        CommandListBaseT::Execute(completed_callback);

        if (!m_is_cmd_buffer_enabled || !m_mtl_cmd_buffer)
            return;

        [m_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
            std::scoped_lock lock_guard(m_cmd_buffer_mutex);
            CommandListBaseT::Complete();
            m_mtl_cmd_buffer  = nil;
        }];

        [m_mtl_cmd_buffer commit];
    }

    // IObject interface

    bool SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();
        std::scoped_lock lock_guard(m_cmd_buffer_mutex);

        if (!CommandListBaseT::SetName(name))
            return false;

        m_ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);

        if (m_mtl_cmd_encoder != nil)
        {
            m_mtl_cmd_encoder.label = m_ns_name;
        }

        if (m_mtl_cmd_buffer != nil)
        {
            m_mtl_cmd_buffer.label = m_ns_name;
        }

        return true;
    }

    const MTLCommandEncoderId&  GetNativeCommandEncoder() const noexcept { return m_mtl_cmd_encoder; }
    const id<MTLCommandBuffer>& GetNativeCommandBuffer() const noexcept { return m_mtl_cmd_buffer; }

    CommandQueue& GetMetalCommandQueue() noexcept
    {
        META_FUNCTION_TASK();
        return static_cast<class CommandQueue&>(CommandListBaseT::GetCommandQueue());
    }

protected:
    const id<MTLCommandBuffer>& InitializeCommandBuffer()
    {
        META_FUNCTION_TASK();
        std::scoped_lock lock_guard(m_cmd_buffer_mutex);

        if (m_mtl_cmd_buffer)
            return m_mtl_cmd_buffer;

        const id<MTLCommandQueue>& mtl_command_queue = GetMetalCommandQueue().GetNativeCommandQueue();
        META_CHECK_ARG_NOT_NULL(mtl_command_queue);

        m_mtl_cmd_buffer = [mtl_command_queue commandBuffer];
        m_mtl_cmd_buffer.label = m_ns_name;

        META_CHECK_ARG_NOT_NULL(m_mtl_cmd_buffer);
        return m_mtl_cmd_buffer;
    }

    void InitializeCommandEncoder(const MTLCommandEncoderId& mtl_cmd_encoder)
    {
        META_FUNCTION_TASK();
        META_CHECK_ARG_NOT_NULL(mtl_cmd_encoder);

        m_mtl_cmd_encoder = mtl_cmd_encoder;
        m_mtl_cmd_encoder.label = m_ns_name;
    }

    bool IsCommandBufferInitialized() const noexcept   { return m_mtl_cmd_buffer; }
    bool IsCommandEncoderInitialized() const noexcept  { return m_mtl_cmd_encoder; }

private:
    const bool           m_is_cmd_buffer_enabled;
    id<MTLCommandBuffer> m_mtl_cmd_buffer = nil;
    MTLCommandEncoderId  m_mtl_cmd_encoder = nil;
    NSString*            m_ns_name = nil;
    mutable TracyLockable(std::mutex, m_cmd_buffer_mutex);
};

} // namespace Methane::Graphics::Metal
