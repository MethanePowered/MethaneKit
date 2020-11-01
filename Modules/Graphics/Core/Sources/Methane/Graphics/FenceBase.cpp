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

FILE: Methane/Graphics/DirectX12/FenceDX.cpp
DirectX 12 fence wrapper.

******************************************************************************/

#include "FenceBase.h"
#include "CommandQueueBase.h"

#include <Methane/Exceptions.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

FenceBase::FenceBase(CommandQueueBase& command_queue)
    : m_command_queue(command_queue)
{
    META_FUNCTION_TASK();
}

void FenceBase::Signal()
{
    META_FUNCTION_TASK();
    META_LOG("GPU SIGNAL fence \"" + GetName() + "\" with value " + std::to_string(m_value + 1));

    m_value++;
}

void FenceBase::WaitOnCpu()
{
    META_FUNCTION_TASK();
    META_LOG("CPU WAIT fence \"" + GetName() + "\" with value " + std::to_string(m_value));
}

void FenceBase::WaitOnGpu(CommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("wait_on_command_queue", std::addressof(wait_on_command_queue) != std::addressof(m_command_queue),
                              "fence can not be waited on GPU at the same command queue where it was signalled");
    META_LOG("GPU WAIT fence \"" + GetName() + "\" on command queue \"" + wait_on_command_queue.GetName() + "\" with value " + std::to_string(m_value));
}

void FenceBase::FlushOnCpu()
{
    META_FUNCTION_TASK();
    Signal();
    WaitOnCpu();
}

void FenceBase::FlushOnGpu(CommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    Signal();
    WaitOnGpu(wait_on_command_queue);
}

} // namespace Methane::Graphics
