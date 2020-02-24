/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ResourceVK.h
Vulkan implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ResourceBase.h>

#include <memory>

namespace Methane::Graphics
{

struct IContextVK;
struct ResourceContainerVK;

class ResourceVK : public ResourceBase
{
public:
    class ReleasePoolVK final : public ReleasePool
    {
    public:
        ReleasePoolVK();

        // ReleasePool interface
        void AddResource(ResourceBase& resource) override;
        void ReleaseResources() override;

    private:
        std::unique_ptr<ResourceContainerVK> m_sp_vk_resources;
    };

    ResourceVK(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage);

protected:
    IContextVK& GetContextVK() noexcept;
};

} // namespace Methane::Graphics
