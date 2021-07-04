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

FILE: Methane/Graphics/Vulkan/CommandListVK.h
Vulkan command lists sequence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandListBase.h>

namespace Methane::Graphics
{

class CommandQueueVK;

struct ICommandListVK
{
    class DebugGroupVK final : public CommandListBase::DebugGroupBase
    {
    public:
        explicit DebugGroupVK(const std::string& name);
    };

    virtual CommandQueueVK&          GetCommandQueueVK() = 0;
    virtual const CommandQueueVK&    GetCommandQueueVK() const = 0;
    virtual const vk::CommandBuffer& GetNativeCommandBuffer() const = 0;
    virtual void SetResourceBarriers(const Resource::Barriers& resource_barriers) = 0;

    virtual ~ICommandListVK() = default;
};

class CommandListSetVK final : public CommandListSetBase
{
public:
    explicit CommandListSetVK(const Refs<CommandList>& command_list_refs);
};

} // namespace Methane::Graphics
