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

FILE: Methane/Graphics/Vulkan/ShaderVK.h
Vulkan implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ShaderBase.h>

#include <string>
#include <memory>

namespace Methane::Graphics
{

class ContextVK;
class ProgramVK;

class ShaderVK : public ShaderBase
{
public:
    class ResourceBindingVK : public ResourceBindingBase
    {
    public:
        struct Settings
        {
            ResourceBindingBase::Settings base;
        };
        
        ResourceBindingVK(ContextBase& context, const Settings& settings);
        ResourceBindingVK(const ResourceBindingVK& other) = default;
        
        // ResourceBinding interface
        void SetResourceLocation(Resource::Location resource_location) override;
        uint32_t GetResourceCount() const override { return 1; }
        
        // ResourceBindingBase interface
        DescriptorHeap::Type GetDescriptorHeapType() const override;
        
        const Settings& GetSettings() const noexcept { return m_settings; }
        
    protected:
        const Settings m_settings;
    };
    
    ShaderVK(Shader::Type shader_type, ContextVK& context, const Settings& settings);
    ~ShaderVK() override;
    
    // ShaderBase interface
    ResourceBindings GetResourceBindings(const std::set<std::string>& constant_argument_names,
                                         const std::set<std::string>& addressable_argument_names) const override;

protected:
    ContextVK& GetContextVK() noexcept;
};

} // namespace Methane::Graphics
