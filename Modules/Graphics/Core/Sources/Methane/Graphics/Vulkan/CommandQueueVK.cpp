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

FILE: Methane/Graphics/Vulkan/CommandQueueVK.mm
Vulkan implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueVK.h"
#include "ContextVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<CommandQueue> CommandQueue::Create(Context& context)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<CommandQueueVK>(dynamic_cast<ContextBase&>(context));
}

CommandQueueVK::CommandQueueVK(ContextBase& context)
    : CommandQueueBase(context)
{
    ITT_FUNCTION_TASK();
}

CommandQueueVK::~CommandQueueVK()
{
    ITT_FUNCTION_TASK();
}

void CommandQueueVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    CommandQueueBase::SetName(name);
}

IContextVK& CommandQueueVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetContext());
}

} // namespace Methane::Graphics
