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

FILE: Methane/Graphics/Vulkan/FenceVK.cpp
Vulkan fence implementation.

******************************************************************************/

#include "FenceVK.h"
#include "CommandQueueVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics
{

UniquePtr<Fence> Fence::Create(CommandQueue& command_queue)
{
    ITT_FUNCTION_TASK();
    return std::make_unique<FenceVK>(static_cast<CommandQueueBase&>(command_queue));
}

FenceVK::FenceVK(CommandQueueBase& command_queue)
    : FenceBase(command_queue)
{
    ITT_FUNCTION_TASK();

    // TODO: create native fence object
}

FenceVK::~FenceVK()
{
    ITT_FUNCTION_TASK();

    // TODO: release native fence object
}

void FenceVK::Signal()
{
    ITT_FUNCTION_TASK();

    FenceBase::Signal();

    // TODO: signal native fence object
}

void FenceVK::Wait()
{
    ITT_FUNCTION_TASK();

    FenceBase::Wait();

    // TODO: wait for native fence object
}

void FenceVK::SetName(const std::string& name) noexcept
{
    ITT_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

   ObjectBase::SetName(name);

    // TODO: set name of native fence object
}

CommandQueueVK& FenceVK::GetCommandQueueVK()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(GetCommandQueue());
}

} // namespace Methane::Graphics
