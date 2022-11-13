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

FILE: Methane/Graphics/Vulkan/ContextMT.h
Vulkan context accessor interface for template class ContextMT<ContextBaseT>

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/ICommandList.h>

namespace Methane::Graphics::Vulkan
{

class Device;
class CommandQueue;
class DescriptorManager;

struct IContextVk
{
    virtual const Device& GetVulkanDevice() const noexcept = 0;
    virtual CommandQueue& GetVulkanDefaultCommandQueue(Rhi::CommandListType type) = 0;
    virtual DescriptorManager& GetVulkanDescriptorManager() const = 0;

    virtual ~IContextVk() = default;
};

} // namespace Methane::Graphics::Vulkan
