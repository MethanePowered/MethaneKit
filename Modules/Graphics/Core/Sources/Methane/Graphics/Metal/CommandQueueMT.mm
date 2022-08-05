/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/CommandQueueMT.mm
Metal implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueMT.hh"
#include "DeviceMT.hh"
#include "RenderContextMT.hh"

#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <QuartzCore/CABase.h>

namespace Methane::Graphics
{

Ptr<CommandQueue> CommandQueue::Create(const Context& context, CommandList::Type command_lists_type)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandQueueMT>(dynamic_cast<const ContextBase&>(context), command_lists_type);
}

CommandQueueMT::CommandQueueMT(const ContextBase& context, CommandList::Type command_lists_type)
    : CommandQueueBase(context, command_lists_type)
    , m_mtl_command_queue([GetContextMT().GetDeviceMT().GetNativeDevice() newCommandQueue])
{
    META_FUNCTION_TASK();
    InitializeTracyGpuContext(
        Tracy::GpuContext::Settings(
            Tracy::GpuContext::Type::Metal,
            0U,
            Data::ConvertTimeSecondsToNanoseconds(CACurrentMediaTime())
        )
    );
}

bool CommandQueueMT::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!CommandQueueBase::SetName(name))
        return false;

    META_CHECK_ARG_NOT_NULL(m_mtl_command_queue);
    m_mtl_command_queue.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
    return true;
}

const IContextMT& CommandQueueMT::GetContextMT() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextMT&>(GetContextBase());
}

const RenderContextMT& CommandQueueMT::GetRenderContextMT() const
{
    META_FUNCTION_TASK();
    const ContextBase& context = GetContextBase();
    META_CHECK_ARG_EQUAL_DESCR(context.GetType(), Context::Type::Render, "incompatible context type");
    return static_cast<const RenderContextMT&>(context);
}

} // namespace Methane::Graphics
