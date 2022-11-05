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

#include <Methane/Graphics/Base/Fence.h>
#include <Methane/Graphics/Base/CommandQueue.h>

#include <Methane/Exceptions.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

Fence::Fence(CommandQueue& command_queue)
    : m_command_queue(command_queue)
{
    META_FUNCTION_TASK();
}

void Fence::Signal()
{
    META_FUNCTION_TASK();
    META_LOG("Fence '{}' SIGNAL from GPU with value {}", GetName(), m_value + 1);

    m_value++;
}

void Fence::WaitOnCpu()
{
    META_FUNCTION_TASK();
    META_LOG("Fence '{}' WAIT on CPU with value {}", GetName(), m_value);
}

void Fence::WaitOnGpu(ICommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    META_UNUSED(wait_on_command_queue);
    META_CHECK_ARG_NAME_DESCR("wait_on_command_queue", std::addressof(wait_on_command_queue) != std::addressof(m_command_queue),
                              "fence can not be waited on GPU at the same command queue where it was signalled");
    META_LOG("Fence '{}' WAIT on GPU command queue '{}' with value {}", GetName(), wait_on_command_queue.GetName(), m_value);
}

void Fence::FlushOnCpu()
{
    META_FUNCTION_TASK();
    Signal();
    WaitOnCpu();
}

void Fence::FlushOnGpu(ICommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    Signal();
    WaitOnGpu(wait_on_command_queue);
}

} // namespace Methane::Graphics::Base
