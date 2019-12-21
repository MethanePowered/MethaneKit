/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ShaderVK.mm
Vulkan implementation of the shader interface.

******************************************************************************/

#include "ShaderVK.h"
#include "ContextVK.h"

#include <Methane/Data/Instrumentation.h>

namespace Methane::Graphics
{

Shader::ResourceBinding::Ptr Shader::ResourceBinding::CreateCopy(const ResourceBinding& other_resource_binging)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderVK::ResourceBindingVK>(static_cast<const ShaderVK::ResourceBindingVK&>(other_resource_binging));
}

ShaderVK::ResourceBindingVK::ResourceBindingVK(ContextBase& context, const Settings& settings)
    : ResourceBindingBase(context, settings.base)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

void ShaderVK::ResourceBindingVK::SetResourceLocation(Resource::Location resource_location)
{
    ITT_FUNCTION_TASK();

    if (!resource_location.sp_resource)
    {
        throw std::invalid_argument("Can not set resource location with Null resource pointer.");
    }
    
    const Resource::Type resource_type = resource_location.sp_resource->GetResourceType();
    
    ShaderBase::ResourceBindingBase::SetResourceLocation(std::move(resource_location));
}

DescriptorHeap::Type ShaderVK::ResourceBindingVK::GetDescriptorHeapType() const
{
    ITT_FUNCTION_TASK();
    return DescriptorHeap::Type::Undefined;
}

Shader::Ptr Shader::Create(Shader::Type shader_type, Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderVK>(shader_type, static_cast<ContextVK&>(context), settings);
}

ShaderVK::ShaderVK(Shader::Type shader_type, ContextVK& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
{
    ITT_FUNCTION_TASK();
}

ShaderVK::~ShaderVK()
{
    ITT_FUNCTION_TASK();
}

ShaderBase::ResourceBindings ShaderVK::GetResourceBindings(const std::set<std::string>& constant_argument_names,
                                                           const std::set<std::string>& addressable_argument_names) const
{
    ITT_FUNCTION_TASK();

    ShaderBase::ResourceBindings resource_bindings;
    return resource_bindings;

    return resource_bindings;
}

ContextVK& ShaderVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextVK&>(m_context);
}

} // namespace Methane::Graphics
