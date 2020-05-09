/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/BlitCommandListMT.mm
Metal implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListMT.hh"
#include "CommandListMT.hh"
#include "CommandQueueMT.hh"
#include "RenderContextMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<BlitCommandListMT>(static_cast<CommandQueueBase&>(command_queue));
}

BlitCommandListMT::BlitCommandListMT(CommandQueueBase& command_queue)
    : CommandListBase(command_queue, CommandList::Type::Blit)
{
    META_FUNCTION_TASK();
}

void BlitCommandListMT::Reset(DebugGroup*)
{
    META_FUNCTION_TASK();
    
    if (m_mtl_blit_encoder)
        return;

    if (!m_mtl_cmd_buffer)
    {
        m_mtl_cmd_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];

        assert(m_mtl_cmd_buffer != nil);
        m_mtl_cmd_buffer.label = m_ns_name;
    }

    assert(m_mtl_cmd_buffer != nil);
    m_mtl_blit_encoder = [m_mtl_cmd_buffer blitCommandEncoder];

    assert(m_mtl_blit_encoder != nil);
    m_mtl_blit_encoder.label = m_ns_name;
}

void BlitCommandListMT::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    CommandListBase::SetName(name);

    m_ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);
    
    if (m_mtl_blit_encoder != nil)
    {
        m_mtl_blit_encoder.label = m_ns_name;
    }
    
    if (m_mtl_cmd_buffer != nil)
    {
        m_mtl_cmd_buffer.label = m_ns_name;
    }
}

void BlitCommandListMT::PushDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();

    CommandListBase::PushDebugGroup(debug_group);

    assert(m_mtl_blit_encoder != nil);
    [m_mtl_blit_encoder pushDebugGroup:static_cast<CommandListMT::DebugGroupMT&>(debug_group).GetNSName()];
}

void BlitCommandListMT::PopDebugGroup()
{
    META_FUNCTION_TASK();

    CommandListBase::PopDebugGroup();

    assert(m_mtl_blit_encoder != nil);
    [m_mtl_blit_encoder popDebugGroup];
}

void BlitCommandListMT::Commit()
{
    META_FUNCTION_TASK();
    
    assert(!IsCommitted());

    CommandListBase::Commit();

    if (m_mtl_blit_encoder)
    {
        [m_mtl_blit_encoder endEncoding];
        m_mtl_blit_encoder = nil;
    }

    if (!m_mtl_cmd_buffer)
        return;

    [m_mtl_cmd_buffer enqueue];
}

Data::TimeRange BlitCommandListMT::GetGpuTimeRange() const
{
    META_FUNCTION_TASK();
    if (GetState() != CommandListBase::State::Pending)
        throw std::logic_error("Can not get GPU time range of executing or not committed command list.");

    assert(m_mtl_cmd_buffer.status == MTLCommandBufferStatusCompleted);
    return Data::TimeRange(
        Data::ConvertTimeSecondsToNanoseconds(m_mtl_cmd_buffer.GPUStartTime),
        Data::ConvertTimeSecondsToNanoseconds(m_mtl_cmd_buffer.GPUEndTime)
    );
}

void BlitCommandListMT::Execute(uint32_t frame_index, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();

    CommandListBase::Execute(frame_index, completed_callback);

    if (!m_mtl_cmd_buffer)
        return;

    [m_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
        Complete(frame_index);
        m_mtl_cmd_buffer  = nil;
    }];

    [m_mtl_cmd_buffer commit];
}

CommandQueueMT& BlitCommandListMT::GetCommandQueueMT() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class CommandQueueMT&>(GetCommandQueue());
}

} // namespace Methane::Graphics
