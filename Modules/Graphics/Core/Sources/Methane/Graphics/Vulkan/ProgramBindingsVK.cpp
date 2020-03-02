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

FILE: Methane/Graphics/Vulkan/ProgramVK.h
Vulkan implementation of the program interface.

******************************************************************************/

#include "ProgramBindingsVK.h"
#include "RenderCommandListVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsVK>(sp_program, resource_locations_by_argument);
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsVK>(static_cast<const ProgramBindingsVK&>(other_program_bindings), replace_resource_location_by_argument);
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsVK::ArgumentBindingVK>(static_cast<const ProgramBindingsVK::ArgumentBindingVK&>(other_argument_binding));
}

ProgramBindingsVK::ArgumentBindingVK::ArgumentBindingVK(const ContextBase& context, SettingsVK settings)
    : ArgumentBindingBase(context, settings)
    , m_settings_vk(std::move(settings))
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsVK::ArgumentBindingVK::SetResourceLocations(const Resource::Locations& resource_locations)
{
    ITT_FUNCTION_TASK();

    ArgumentBindingBase::SetResourceLocations(resource_locations);
}

ProgramBindingsVK::ProgramBindingsVK(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
    : ProgramBindingsBase(sp_program, resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

ProgramBindingsVK::ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument)
    : ProgramBindingsBase(other_program_bindings, replace_resource_location_by_argument)
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsVK::Apply(CommandListBase& command_list, ApplyBehavior::Mask apply_behavior) const
{
    ITT_FUNCTION_TASK();

    RenderCommandListVK& vulkan_command_list = static_cast<RenderCommandListVK&>(command_list);

    for(const auto& binding_by_argument : GetArgumentBindings())
    {
        const ProgramBindingsVK::ArgumentBindingVK& vulkan_argument_binding = static_cast<const ProgramBindingsVK::ArgumentBindingVK&>(*binding_by_argument.second);
        if ((apply_behavior & ApplyBehavior::ConstantOnce || apply_behavior & ApplyBehavior::ChangesOnly) && vulkan_command_list.GetProgramBindings() &&
            vulkan_argument_binding.IsAlreadyApplied(GetProgram(), *vulkan_command_list.GetProgramBindings(), apply_behavior & ApplyBehavior::ChangesOnly))
            continue;
    }
}

} // namespace Methane::Graphics
