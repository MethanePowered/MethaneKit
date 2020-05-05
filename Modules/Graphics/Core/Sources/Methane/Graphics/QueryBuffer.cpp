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

FILE: Methane/Graphics/DirectX12/QueryBuffer.cpp
GPU data query buffer base implementation.

******************************************************************************/

#include "QueryBuffer.h"
#include "CommandQueueBase.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

QueryBuffer::QueryBuffer(CommandQueueBase& command_queue, Type type)
    : m_type(type)
    , m_command_queue(command_queue)
    , m_context(dynamic_cast<Context&>(command_queue.GetContext()))
{
    META_FUNCTION_TASK();
}

TimestampQueryBufferDummy::TimestampQueryBufferDummy(CommandQueueBase& command_queue, uint32_t)
    : QueryBuffer(command_queue, Type::Timestamp)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics