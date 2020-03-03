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

FILE: Methane/Graphics/Metal/FenceMT.mm
Metal fence implementation.

******************************************************************************/

#include "FenceMT.hh"
#include "CommandQueueMT.hh"
#include "DeviceMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

UniquePtr<Fence> Fence::Create(CommandQueue& command_queue)
{
    ITT_FUNCTION_TASK();
    return std::make_unique<FenceMT>(static_cast<CommandQueueBase&>(command_queue));
}
    
dispatch_queue_t& FenceMT::GetDispatchQueue()
{
    static dispatch_queue_t s_fences_dispatch_queue = dispatch_queue_create("com.example.methane.fences", NULL);
    return s_fences_dispatch_queue;
}

FenceMT::FenceMT(CommandQueueBase& command_queue)
    : FenceBase(command_queue)
    , m_mtl_event([GetCommandQueueMT().GetContextMT().GetDeviceMT().GetNativeDevice() newSharedEvent])
    , m_mtl_event_listener([[MTLSharedEventListener alloc] initWithDispatchQueue:GetDispatchQueue()])
{
    ITT_FUNCTION_TASK();
}

FenceMT::~FenceMT()
{
    ITT_FUNCTION_TASK();
    [m_mtl_event_listener release];
}

void FenceMT::Signal()
{
    ITT_FUNCTION_TASK();

    FenceBase::Signal();
    
    id<MTLCommandBuffer> mtl_command_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];
    [mtl_command_buffer encodeSignalEvent:m_mtl_event value:GetValue()];
    [mtl_command_buffer commit];
    
    m_is_signalled = false;
}

void FenceMT::Wait()
{
    ITT_FUNCTION_TASK();

    FenceBase::Wait();

    assert(m_mtl_event != nil);
    assert(m_mtl_event_listener != nil);
    uint64_t signalled_value = m_mtl_event.signaledValue;
    if (signalled_value >= GetValue())
        return;
    
    assert(!m_is_signalled);
    [m_mtl_event notifyListener:m_mtl_event_listener
                        atValue:GetValue()
                          block:^(id<MTLSharedEvent>, uint64_t /*value*/)
                                {
                                    m_is_signalled = true;
                                    m_wait_condition_var.notify_one();
                                }];
    std::unique_lock<std::mutex> lock(m_wait_mutex);
    m_wait_condition_var.wait(lock, [this]{ return m_is_signalled; });
}

void FenceMT::SetName(const std::string& name) noexcept
{
    ITT_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

   ObjectBase::SetName(name);

    m_mtl_event.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
}

CommandQueueMT& FenceMT::GetCommandQueueMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueMT&>(GetCommandQueue());
}

} // namespace Methane::Graphics
