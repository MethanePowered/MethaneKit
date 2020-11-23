/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ResourceVK.h
Vulkan implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Instrumentation.h>

#include <memory>

namespace Methane::Graphics
{

struct IContextVK;

class ResourceBarriersVK : public ResourceBase::Barriers
{
public:
    explicit ResourceBarriersVK(const Set& barriers) : ResourceBase::Barriers(barriers) {}
};

template<typename ReourceBaseType, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ReourceBaseType>, void>>
class ResourceVK : public ReourceBaseType
{
public:
    template<typename SettingsType>
    ResourceVK(ContextBase& context, const SettingsType& settings, const ResourceBase::DescriptorByUsage& descriptor_by_usage)
        : ReourceBaseType(context, settings, descriptor_by_usage)
    {
        META_FUNCTION_TASK();
    }

protected:
    IContextVK& GetContextVK() noexcept
    {
        return dynamic_cast<IContextVK&>(ResourceBase::GetContextBase());
    }
};

} // namespace Methane::Graphics
