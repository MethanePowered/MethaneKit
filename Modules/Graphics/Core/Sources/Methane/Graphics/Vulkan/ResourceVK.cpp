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

FILE: Methane/Graphics/Vulkan/ResourceVK.mm
Vulkan implementation of the resource interface.

******************************************************************************/

#include "ResourceVK.h"
#include "ContextVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

struct ResourceContainerVK
{
};

Ptr<ResourceBase::ReleasePool> ResourceBase::ReleasePool::Create()
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ResourceVK::ReleasePoolVK>();
}

ResourceVK::ReleasePoolVK::ReleasePoolVK()
    : ResourceBase::ReleasePool()
    , m_sp_vk_resources(new ResourceContainerVK())
{
    ITT_FUNCTION_TASK();
}

void ResourceVK::ReleasePoolVK::AddResource(ResourceBase& resource)
{
    ITT_FUNCTION_TASK();
}

void ResourceVK::ReleasePoolVK::ReleaseResources()
{
    ITT_FUNCTION_TASK();
    m_sp_vk_resources.reset(new ResourceContainerVK());
}

ResourceVK::ResourceVK(Type type, Usage::Mask usage_mask, ContextBase& context, const DescriptorByUsage& descriptor_by_usage)
    : ResourceBase(type, usage_mask, context, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
}

IContextVK& ResourceVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetContext());
}

} // namespace Methane::Graphics
