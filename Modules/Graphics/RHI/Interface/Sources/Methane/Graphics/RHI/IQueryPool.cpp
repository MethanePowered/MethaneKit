/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IQueryPool.cpp
Methane query pool interface.

******************************************************************************/

#include <Methane/Graphics/RHI/IQueryPool.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<ITimestampQueryPool> ITimestampQueryPool::Create(ICommandQueue& command_queue, uint32_t max_timestamps_per_frame)
{
    META_FUNCTION_TASK();
    return command_queue.CreateTimestampQueryPool(max_timestamps_per_frame);
}

} // namespace Methane::Graphics::Rhi
