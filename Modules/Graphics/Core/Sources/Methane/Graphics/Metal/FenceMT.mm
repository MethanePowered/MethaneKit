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

FILE: Methane/Graphics/Metal/FenceMT.mm
Metal fence implementation.

******************************************************************************/

#include "FenceMT.hh"
#include "CommandQueueMT.hh"
#include "DeviceMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/ScopeTimer.h>

namespace Methane::Graphics
{

Ptr<Fence> Fence::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<FenceMT>(static_cast<CommandQueueBase&>(command_queue));
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
    META_FUNCTION_TASK();
}

FenceMT::~FenceMT()
{
    META_FUNCTION_TASK();
    [m_mtl_event_listener release];
}

void FenceMT::Signal()
{
    META_FUNCTION_TASK();
    FenceBase::Signal();
    
    id<MTLCommandBuffer> mtl_command_buffer = [GetCommandQueueMT().GetNativeCommandQueue() commandBuffer];
    [mtl_command_buffer encodeSignalEvent:m_mtl_event value:GetValue()];
    [mtl_command_buffer commit];
    
    m_is_signalled = false;
}

void FenceMT::WaitOnCpu()
{
    META_FUNCTION_TASK();
    FenceBase::WaitOnCpu();

    META_CHECK_ARG_NOT_NULL(m_mtl_event);
    uint64_t signalled_value = m_mtl_event.signaledValue;
    if (signalled_value >= GetValue())
        return;

    META_CHECK_ARG_FALSE(m_is_signalled);
    META_CHECK_ARG_NOT_NULL(m_mtl_event_listener);
    [m_mtl_event notifyListener:m_mtl_event_listener
                        atValue:GetValue()
                          block:^(id<MTLSharedEvent>, uint64_t /*value*/)
                                {
                                    m_is_signalled = true;
                                    m_wait_condition_var.notify_one();
                                }];
    std::unique_lock lock(m_wait_mutex);
    m_wait_condition_var.wait(lock, [this]{ return m_is_signalled; });
}

void FenceMT::WaitOnGpu(CommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    FenceBase::WaitOnGpu(wait_on_command_queue);

    CommandQueueMT& mtl_wait_on_command_queue = static_cast<CommandQueueMT&>(wait_on_command_queue);
    id<MTLCommandBuffer> mtl_command_buffer = [mtl_wait_on_command_queue.GetNativeCommandQueue() commandBuffer];
    [mtl_command_buffer encodeWaitForEvent:m_mtl_event value:GetValue()];
    [mtl_command_buffer commit];
}

void FenceMT::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

   ObjectBase::SetName(name);

    m_mtl_event.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
}

CommandQueueMT& FenceMT::GetCommandQueueMT()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueMT&>(GetCommandQueue());
}

} // namespace Methane::Graphics
