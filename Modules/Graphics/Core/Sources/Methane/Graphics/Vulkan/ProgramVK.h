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

FILE: Methane/Graphics/Vulkan/ProgramVK.h
Vulkan implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ProgramBase.h>

namespace Methane::Graphics
{

class ContextVK;
class ShaderVK;

class ProgramVK : public ProgramBase
{
public:
    class ResourceBindingsVK : public ResourceBindingsBase
    {
    public:
        ResourceBindingsVK(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument);
        ResourceBindingsVK(const ResourceBindingsVK& other_resource_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument);

        // ResourceBindings interface
        void Apply(CommandList& command_list, ApplyBehavior::Mask apply_behavior) const override;
        
        // ResourceBindingsBase interface
        void CompleteInitialization() override { }
    };

    ProgramVK(ContextBase& context, const Settings& settings);
    ~ProgramVK() override;

    ShaderVK& GetShaderVK(Shader::Type shader_type) noexcept;

protected:
    ContextVK& GetContextVK() noexcept;
};

} // namespace Methane::Graphics
