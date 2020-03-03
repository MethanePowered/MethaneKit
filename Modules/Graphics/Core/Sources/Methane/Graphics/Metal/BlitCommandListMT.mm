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
#include "CommandQueueMT.hh"
#include "RenderContextMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& command_queue)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<BlitCommandListMT>(static_cast<CommandQueueBase&>(command_queue));
}

BlitCommandListMT::BlitCommandListMT(CommandQueueBase& command_queue)
    : CommandListBase(command_queue, CommandList::Type::Blit)
{
    ITT_FUNCTION_TASK();
}

void BlitCommandListMT::Reset(const std::string& debug_group)
{
    ITT_FUNCTION_TASK();
    if (m_mtl_blit_encoder)
        return;

    if (!m_mtl_cmd_buffer)
    {
        m_mtl_cmd_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];
        assert(m_mtl_cmd_buffer != nil);

        m_mtl_cmd_buffer.label = MacOS::ConvertToNsType<std::string, NSString*>(GetName());
    }

    assert(m_mtl_cmd_buffer != nil);
    m_mtl_blit_encoder = [m_mtl_cmd_buffer blitCommandEncoder];

    assert(m_mtl_blit_encoder != nil);
    m_mtl_blit_encoder.label = MacOS::ConvertToNsType<std::string, NSString*>(GetName());
}

void BlitCommandListMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    CommandListBase::SetName(name);
    
    NSString* ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);
    
    if (m_mtl_blit_encoder != nil)
    {
        m_mtl_blit_encoder.label = ns_name;
    }
    
    if (m_mtl_cmd_buffer != nil)
    {
        m_mtl_cmd_buffer.label = ns_name;
    }
}

void BlitCommandListMT::PushDebugGroup(const std::string& name)
{
    ITT_FUNCTION_TASK();

    assert(m_mtl_blit_encoder != nil);
    NSString* ns_name = MacOS::ConvertToNsType<std::string, NSString*>(name);
    [m_mtl_blit_encoder pushDebugGroup:ns_name];
}

void BlitCommandListMT::PopDebugGroup()
{
    ITT_FUNCTION_TASK();

    assert(m_mtl_blit_encoder != nil);
    [m_mtl_blit_encoder popDebugGroup];
}

void BlitCommandListMT::Commit()
{
    ITT_FUNCTION_TASK();
    
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

void BlitCommandListMT::Execute(uint32_t frame_index)
{
    ITT_FUNCTION_TASK();

    CommandListBase::Execute(frame_index);

    if (!m_mtl_cmd_buffer)
        return;

    [m_mtl_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
        Complete(frame_index);
    }];

    [m_mtl_cmd_buffer commit];
    m_mtl_cmd_buffer  = nil;
}

CommandQueueMT& BlitCommandListMT::GetCommandQueueMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class CommandQueueMT&>(GetCommandQueue());
}

} // namespace Methane::Graphics
