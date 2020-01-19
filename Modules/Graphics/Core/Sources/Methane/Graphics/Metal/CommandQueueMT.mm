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

FILE: Methane/Graphics/Metal/CommandQueueMT.mm
Metal implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueMT.hh"
#include "DeviceMT.hh"
#include "ContextMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<CommandQueue> CommandQueue::Create(Context& context)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<CommandQueueMT>(static_cast<ContextBase&>(context));
}

CommandQueueMT::CommandQueueMT(ContextBase& context)
    : CommandQueueBase(context, true)
    , m_mtl_command_queue([GetContextMT().GetDeviceMT().GetNativeDevice() newCommandQueue])
{
    ITT_FUNCTION_TASK();
}

CommandQueueMT::~CommandQueueMT()
{
    ITT_FUNCTION_TASK();
    assert(!IsExecuting());

    [m_mtl_command_queue release];
}

void CommandQueueMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    CommandQueueBase::SetName(name);

    assert(m_mtl_command_queue != nil);
    m_mtl_command_queue.label = MacOS::ConvertToNSType<std::string, NSString*>(name);
}

ContextMT& CommandQueueMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextMT&>(m_context);
}

} // namespace Methane::Graphics
