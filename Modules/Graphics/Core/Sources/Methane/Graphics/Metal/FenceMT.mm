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

FILE: Methane/Graphics/Metal/FenceMT.cpp
Metal fence implementation.

******************************************************************************/

#include "FenceMT.hh"
#include "CommandQueueMT.hh"
#include "DeviceMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

UniquePtr<Fence> Fence::Create(CommandQueue& command_queue)
{
    ITT_FUNCTION_TASK();
    return std::make_unique<FenceMT>(static_cast<CommandQueueBase&>(command_queue));
}

FenceMT::FenceMT(CommandQueueBase& command_queue)
    : FenceBase(command_queue)
{
    ITT_FUNCTION_TASK();

    // TODO: create native fence object
}

FenceMT::~FenceMT()
{
    ITT_FUNCTION_TASK();

    // TODO: release native fence object
}

void FenceMT::Signal()
{
    ITT_FUNCTION_TASK();

    FenceBase::Signal();

    // TODO: signal native fence object
}

void FenceMT::Wait()
{
    ITT_FUNCTION_TASK();

    FenceBase::Wait();

    // TODO: wait for native fence object
}

void FenceMT::SetName(const std::string& name) noexcept
{
    ITT_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

   ObjectBase::SetName(name);

    // TODO: set name of native fence object
}

CommandQueueMT& FenceMT::GetCommandQueueMT()
{
    ITT_FUNCTION_TASK();
    return static_cast<CommandQueueMT&>(GetCommandQueue());
}

} // namespace Methane::Graphics
