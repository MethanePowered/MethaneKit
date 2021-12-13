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

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/CoreFormatters.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

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

void ProgramBindingsVK::ArgumentBindingVK::SetDescriptorSetBinding(const vk::DescriptorSet& descriptor_set, uint32_t layout_binding_index) noexcept
{
    META_FUNCTION_TASK();
    m_vk_descriptor_set_ptr   = &descriptor_set;
    m_vk_layout_binding_index = layout_binding_index;
}

void ProgramBindingsVK::ArgumentBindingVK::SetDescriptorSet(const vk::DescriptorSet& descriptor_set) noexcept
{
    META_FUNCTION_TASK();
    m_vk_descriptor_set_ptr = &descriptor_set;
}

void ProgramBindingsVK::ArgumentBindingVK::SetResourceLocations(const Resource::Locations& resource_locations)
{
    META_FUNCTION_TASK();
    if (GetResourceLocations() == resource_locations)
        return;

    ArgumentBindingBase::SetResourceLocations(resource_locations);

    META_CHECK_ARG_NOT_NULL(m_vk_descriptor_set_ptr);

    std::vector<vk::WriteDescriptorSet> vk_write_descriptor_sets;
    for(const Resource::Location& resource_location : resource_locations)
    {
        const vk::DescriptorBufferInfo* p_buffer_info       = nullptr; // TODO: add buffers support
        const vk::DescriptorImageInfo*  p_image_info        = nullptr; // TODO: add images support
        const vk::BufferView*           p_texel_buffer_view = nullptr; // TODO: add texel buffer views

        vk_write_descriptor_sets.emplace_back(
            *m_vk_descriptor_set_ptr,
            m_vk_layout_binding_index,
            resource_location.GetSubresourceIndex().GetArrayIndex(),
            1U,
            m_settings_vk.descriptor_type,
            p_image_info,
            p_buffer_info,
            p_texel_buffer_view
        );
    }

    const auto& vulkan_context = dynamic_cast<const IContextVK&>(GetContext());
    vulkan_context.GetDeviceVK().GetNativeDevice().updateDescriptorSets(vk_write_descriptor_sets, {});
}

ProgramBindingsVK::ProgramBindingsVK(const Ptr<Program>& program_ptr,
                                     const ResourceLocationsByArgument& resource_locations_by_argument,
                                     Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, frame_index)
{
    META_FUNCTION_TASK();
    auto& program = static_cast<ProgramVK&>(GetProgram());
    const vk::DescriptorSetLayout& vk_mutable_descriptor_set_layout = program.GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::Mutable);
    if (vk_mutable_descriptor_set_layout)
    {
        DescriptorManagerVK& descriptor_manager = program.GetContextVK().GetDescriptorManagerVK();
        m_vk_mutable_descriptor_set = descriptor_manager.AllocDescriptorSet(vk_mutable_descriptor_set_layout);
    }

    // Initialize each argument binding with descriptor set pointer and binding index
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        auto& argument_binding = static_cast<ArgumentBindingVK&>(*argument_binding_ptr);
        const ArgumentBindingVK::SettingsVK& argument_binding_settings = argument_binding.GetSettingsVK();
        const Program::ArgumentAccessor::Type access_type = argument_binding_settings.argument.GetAccessorType();

        const ProgramVK::DescriptorSetLayoutInfo& layout_info = program.GetDescriptorSetLayoutInfo(access_type);
        const auto layout_argument_it = std::find(layout_info.arguments.begin(), layout_info.arguments.end(), program_argument);
        META_CHECK_ARG_TRUE_DESCR(layout_argument_it != layout_info.arguments.end(), "unable to find argument '{}' in descriptor set layout", program_argument);
        const uint32_t layout_binding_index = static_cast<uint32_t>(std::distance(layout_info.arguments.begin(), layout_argument_it));

        switch(access_type)
        {
        case Program::ArgumentAccessor::Type::Constant:
            argument_binding.SetDescriptorSetBinding(program.GetConstantDescriptorSet(), layout_binding_index);
            break;

        case Program::ArgumentAccessor::Type::FrameConstant:
            argument_binding.SetDescriptorSetBinding(program.GetFrameConstantDescriptorSet(frame_index), layout_binding_index);
            break;

        case Program::ArgumentAccessor::Type::Mutable:
            argument_binding.SetDescriptorSetBinding(m_vk_mutable_descriptor_set, layout_binding_index);
            break;
        }
    }

    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsVK::ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings,
                                     const ResourceLocationsByArgument& replace_resource_location_by_argument,
                                     const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, frame_index)
{
    META_FUNCTION_TASK();

    if (other_program_bindings.m_vk_mutable_descriptor_set)
    {
        // Allocate new mutable descriptor set
        auto& program = static_cast<ProgramVK&>(GetProgram());
        const vk::DescriptorSetLayout& vk_mutable_desc_set_layout = program.GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::Mutable);
        META_CHECK_ARG_NOT_NULL(vk_mutable_desc_set_layout);
        vk::DescriptorSet copy_mutable_descriptor_set = program.GetContextVK().GetDescriptorManagerVK().AllocDescriptorSet(vk_mutable_desc_set_layout);

        // Copy descriptors from original to new mutable descriptor set
        const vk::Device& vk_device = program.GetContextVK().GetDeviceVK().GetNativeDevice();
        const ProgramVK::DescriptorSetLayoutInfo& mutable_desc_set_layout_info = program.GetDescriptorSetLayoutInfo(Program::ArgumentAccessor::Type::Mutable);
        vk_device.updateDescriptorSets({}, {
            vk::CopyDescriptorSet(other_program_bindings.m_vk_mutable_descriptor_set, {}, {}, copy_mutable_descriptor_set, {}, mutable_desc_set_layout_info.descriptors_count)
        });

        m_vk_mutable_descriptor_set = copy_mutable_descriptor_set;
    }

    // Update mutable argument bindings with a pointer to the copied descriptor set
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        auto& argument_binding = static_cast<ArgumentBindingVK&>(*argument_binding_ptr);
        if (argument_binding.GetSettingsVK().argument.GetAccessorType() != Program::ArgumentAccessor::Type::Mutable)
            continue;

        argument_binding.SetDescriptorSet(m_vk_mutable_descriptor_set);
    }

    SetResourcesForArguments(ReplaceResourceLocations(other_program_bindings.GetArgumentBindings(), replace_resource_location_by_argument));
    VerifyAllArgumentsAreBoundToResources();
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
