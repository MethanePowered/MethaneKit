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

FILE: Methane/Graphics/Vulkan/ProgramVK.h
Vulkan implementation of the program interface.

******************************************************************************/

#include "ProgramBindingsVK.h"
#include "RenderCommandListVK.h"

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsVK>(program_ptr, resource_locations_by_argument, frame_index);
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsVK>(static_cast<const ProgramBindingsVK&>(other_program_bindings), replace_resource_location_by_argument, frame_index);
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsVK::ArgumentBindingVK>(static_cast<const ProgramBindingsVK::ArgumentBindingVK&>(other_argument_binding));
}

ProgramBindingsVK::ArgumentBindingVK::ArgumentBindingVK(const ContextBase& context, const SettingsVK& settings)
    : ArgumentBindingBase(context, settings)
    , m_settings_vk(settings)
{
    META_FUNCTION_TASK();
}

void ProgramBindingsVK::ArgumentBindingVK::SetResourceLocations(const Resource::Locations& resource_locations)
{
    META_FUNCTION_TASK();

    ArgumentBindingBase::SetResourceLocations(resource_locations);
}

ProgramBindingsVK::ProgramBindingsVK(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, resource_locations_by_argument, frame_index)
{
    META_FUNCTION_TASK();
}

ProgramBindingsVK::ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument, const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, replace_resource_location_by_argument, frame_index)
{
    META_FUNCTION_TASK();
}

void ProgramBindingsVK::Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    auto& vulkan_command_list = static_cast<RenderCommandListVK&>(command_list);

    for(const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        if ((magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ConstantOnce) ||
             magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ChangesOnly)) &&
             vulkan_command_list.GetProgramBindings() &&
             static_cast<const ProgramBindingsVK::ArgumentBindingVK&>(*argument_binding_ptr).IsAlreadyApplied(
                 GetProgram(), *vulkan_command_list.GetProgramBindings(),
                 magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ChangesOnly)))
            continue;
    }
}

} // namespace Methane::Graphics
