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

FILE: Methane/Graphics/Vulkan/ICommandList.h
Vulkan command list interface implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IResourceBarriers.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics::Vulkan
{

enum class CommandBufferType : uint32_t
{
    Primary,             // Primary command buffer with no-render commands, like pipeline barriers, executed before render pass begin
    SecondaryRenderPass, // Secondary command buffer with render pass only commands, excluding pipeline barriers
};

class CommandQueue;
class CommandListDebugGroup;

struct ICommandList
{
    using DebugGroup = CommandListDebugGroup;

    virtual CommandQueue& GetVulkanCommandQueue() = 0;
    virtual const CommandQueue& GetVulkanCommandQueue() const = 0;
    virtual const vk::CommandBuffer& GetNativeCommandBufferDefault() const = 0;
    virtual const vk::CommandBuffer& GetNativeCommandBuffer(CommandBufferType cmd_buffer_type = CommandBufferType::Primary) const = 0;
    virtual vk::PipelineBindPoint GetNativePipelineBindPoint() const = 0;
    virtual void SetResourceBarriers(const Rhi::IResourceBarriers& resource_barriers) = 0;

    virtual ~ICommandList() = default;
};

} // namespace Methane::Graphics::Vulkan
