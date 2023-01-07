/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/TransferCommandList.cpp
Vulkan implementation of the transfer command list interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/TransferCommandList.h>
#include <Methane/Graphics/Vulkan/CommandQueue.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Vulkan
{

TransferCommandList::TransferCommandList(CommandQueue& command_queue)
    : CommandList(vk::CommandBufferLevel::ePrimary, {}, command_queue, Rhi::CommandListType::Transfer)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics::Vulkan
