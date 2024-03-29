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

FILE: Methane/Graphics/Null/ComputeCommandList.cpp
Null implementation of the compute command list interface.

******************************************************************************/

#include "Methane/Graphics/Base/ComputeCommandList.h"
#include <Methane/Graphics/Null/ComputeCommandList.h>
#include <Methane/Graphics/Null/CommandQueue.h>

namespace Methane::Graphics::Null
{

ComputeCommandList::ComputeCommandList(CommandQueue& command_queue)
    : CommandList(command_queue)
{ }

void ComputeCommandList::Dispatch(const Rhi::ThreadGroupsCount& thread_groups_count)
{
    m_dispatched_thread_groups_count = thread_groups_count;
    Base::ComputeCommandList::Dispatch(thread_groups_count);
}

} // namespace Methane::Graphics::Null
