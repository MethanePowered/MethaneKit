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

FILE: Methane/Graphics/Vulkan/ComputeCommandList.cpp
Vulkan implementation of the compute command list interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ComputeCommandList.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Vulkan
{

ComputeCommandList::ComputeCommandList(CommandQueue& command_queue)
    : CommandList(vk::CommandBufferLevel::ePrimary, {}, command_queue)
{ }

void ComputeCommandList::Dispatch(const Rhi::ThreadGroupsCount& thread_groups_count)
{
    META_FUNCTION_TASK();
    GetNativeCommandBufferDefault().dispatch(thread_groups_count.GetWidth(), thread_groups_count.GetHeight(), thread_groups_count.GetDepth());
}

} // namespace Methane::Graphics::Vulkan
