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

FILE: Methane/Graphics/Vulkan/ProgramVK.h
Vulkan implementation of the program interface.

******************************************************************************/

#include "ProgramBindingsVK.h"
#include "ProgramVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "CommandListVK.h"
#include "DescriptorManagerVK.h"

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

ProgramBindingsVK::ProgramBindingsVK(const Ptr<Program>& program_ptr,
                                     const ResourceLocationsByArgument& resource_locations_by_argument,
                                     Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, resource_locations_by_argument, frame_index)
{
    META_FUNCTION_TASK();
    auto& program = static_cast<ProgramVK&>(GetProgram());
    m_descriptor_set_by_access_type[*magic_enum::enum_index(Program::ArgumentAccessor::Type::Constant)] = program.GetConstantDescriptorSet();
    m_descriptor_set_by_access_type[*magic_enum::enum_index(Program::ArgumentAccessor::Type::FrameConstant)] = program.GetFrameConstantDescriptorSet(frame_index);

    const vk::DescriptorSetLayout& vk_mutable_descriptor_set_layout = program.GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::Mutable);
    if (!vk_mutable_descriptor_set_layout)
        return;

    DescriptorManagerVK& descriptor_manager = program.GetContextVK().GetDescriptorManagerVK();
    m_descriptor_set_by_access_type[*magic_enum::enum_index(Program::ArgumentAccessor::Type::Mutable)] = descriptor_manager.AllocDescriptorSet(vk_mutable_descriptor_set_layout);
}

ProgramBindingsVK::ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings,
                                     const ResourceLocationsByArgument& replace_resource_location_by_argument,
                                     const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, replace_resource_location_by_argument, frame_index)
{
    META_FUNCTION_TASK();
    std::copy(other_program_bindings.m_descriptor_set_by_access_type.begin(),
              other_program_bindings.m_descriptor_set_by_access_type.end(),
              m_descriptor_set_by_access_type.begin());

    vk::DescriptorSet& mutable_descriptor_set = m_descriptor_set_by_access_type[*magic_enum::enum_index(Program::ArgumentAccessor::Type::Mutable)];
    if (mutable_descriptor_set)
    {
        //  Allocate new mutable descriptor set
        auto& program = static_cast<ProgramVK&>(GetProgram());
        const vk::DescriptorSetLayout& vk_mutable_desc_set_layout = program.GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::Mutable);
        META_CHECK_ARG_NOT_NULL(vk_mutable_desc_set_layout);
        vk::DescriptorSet mutable_copy_descriptor_set = program.GetContextVK().GetDescriptorManagerVK().AllocDescriptorSet(vk_mutable_desc_set_layout);

        // Copy descriptors from original to new mutable descriptor set
        const vk::Device& vk_device = program.GetContextVK().GetDeviceVK().GetNativeDevice();
        const ProgramVK::DescriptorSetLayoutInfo& mutable_desc_set_layout_info = program.GetNativeDescriptorSetLayoutInfo(Program::ArgumentAccessor::Type::Mutable);
        vk_device.updateDescriptorSets({}, {
            vk::CopyDescriptorSet(mutable_descriptor_set, {}, {}, mutable_copy_descriptor_set, {}, mutable_desc_set_layout_info.descriptors_count)
        });

        mutable_descriptor_set = mutable_copy_descriptor_set;
    }
}

void ProgramBindingsVK::Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    Apply(dynamic_cast<ICommandListVK&>(command_list), command_list.GetProgramBindings().get(), apply_behavior);
}

void ProgramBindingsVK::Apply(ICommandListVK& command_list, const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    META_UNUSED(command_list);
    using namespace magic_enum::bitwise_operators;

    for(const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        if ((magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ConstantOnce) ||
             magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ChangesOnly)) &&
            p_applied_program_bindings &&
             static_cast<const ProgramBindingsVK::ArgumentBindingVK&>(*argument_binding_ptr).IsAlreadyApplied(
                 GetProgram(), *p_applied_program_bindings,
                 magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::ChangesOnly)))
            continue;
    }
}

} // namespace Methane::Graphics
