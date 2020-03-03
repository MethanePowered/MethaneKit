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
#include "RenderContextMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<CommandQueue> CommandQueue::Create(Context& context)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<CommandQueueMT>(dynamic_cast<ContextBase&>(context));
}

CommandQueueMT::CommandQueueMT(ContextBase& context)
    : CommandQueueBase(context)
    , m_mtl_command_queue([GetContextMT().GetDeviceMT().GetNativeDevice() newCommandQueue])
{
    ITT_FUNCTION_TASK();
}

CommandQueueMT::~CommandQueueMT()
{
    ITT_FUNCTION_TASK();
    [m_mtl_command_queue release];
}

void CommandQueueMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    CommandQueueBase::SetName(name);

    assert(m_mtl_command_queue != nil);
    m_mtl_command_queue.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
}

IContextMT& CommandQueueMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextMT&>(GetContext());
}

RenderContextMT& CommandQueueMT::GetRenderContextMT()
{
    ITT_FUNCTION_TASK();
    ContextBase& context = GetContext();
    if (context.GetType() != Context::Type::Render)
    {
        throw std::runtime_error("Incompatible context type.");
    }
    return static_cast<RenderContextMT&>(context);
}

} // namespace Methane::Graphics
