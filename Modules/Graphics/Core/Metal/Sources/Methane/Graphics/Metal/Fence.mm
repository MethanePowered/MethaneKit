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

FILE: Methane/Graphics/Metal/Fence.mm
Metal fence implementation.

******************************************************************************/

#include <Methane/Graphics/Metal/Fence.hh>
#include <Methane/Graphics/Metal/CommandQueue.hh>
#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/Context.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/ScopeTimer.h>

namespace Methane::Graphics
{

Ptr<IFence> IFence::Create(ICommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::Fence>(static_cast<Base::CommandQueue&>(command_queue));
}

} // namespace Methane::Graphics

namespace Methane::Graphics::Metal
{
    
const dispatch_queue_t& Fence::GetDispatchQueue()
{
    static const dispatch_queue_t s_fences_dispatch_queue = dispatch_queue_create("com.example.methane.fences", NULL);
    return s_fences_dispatch_queue;
}

Fence::Fence(Base::CommandQueue& command_queue)
    : Base::Fence(command_queue)
    , m_mtl_event([GetMetalCommandQueue().GetMetalContext().GetMetalDevice().GetNativeDevice() newSharedEvent])
    , m_mtl_event_listener([[MTLSharedEventListener alloc] initWithDispatchQueue:GetDispatchQueue()])
{
    META_FUNCTION_TASK();
}

void Fence::Signal()
{
    META_FUNCTION_TASK();
    Base::Fence::Signal();
    
    id<MTLCommandBuffer> mtl_command_buffer = [GetMetalCommandQueue().GetNativeCommandQueue() commandBuffer];
    [mtl_command_buffer encodeSignalEvent:m_mtl_event value:GetValue()];
    [mtl_command_buffer commit];
    
    m_is_signalled = false;
}

void Fence::WaitOnCpu()
{
    META_FUNCTION_TASK();
    Base::Fence::WaitOnCpu();

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

void Fence::WaitOnGpu(ICommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    Base::Fence::WaitOnGpu(wait_on_command_queue);

    CommandQueue& mtl_wait_on_command_queue = static_cast<CommandQueue&>(wait_on_command_queue);
    id<MTLCommandBuffer> mtl_command_buffer = [mtl_wait_on_command_queue.GetNativeCommandQueue() commandBuffer];
    [mtl_command_buffer encodeWaitForEvent:m_mtl_event value:GetValue()];
    [mtl_command_buffer commit];
}

bool Fence::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!Base::Object::SetName(name))
        return false;

    m_mtl_event.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
    return true;
}

CommandQueue& Fence::GetMetalCommandQueue()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetCommandQueue());
}

} // namespace Methane::Graphics::Metal
