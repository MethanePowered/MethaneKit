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
#include "UtilsVK.hpp"

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
    Ptr<ProgramBindingsVK> program_bindings_ptr = std::make_shared<ProgramBindingsVK>(program_ptr, resource_locations_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    Ptr<ProgramBindingsVK> program_bindings_ptr = std::make_shared<ProgramBindingsVK>(static_cast<const ProgramBindingsVK&>(other_program_bindings), replace_resource_location_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
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

void ProgramBindingsVK::ArgumentBindingVK::SetDescriptorSetBinding(const vk::DescriptorSet& descriptor_set, uint32_t binding_value) noexcept
{
    META_FUNCTION_TASK();
    m_vk_descriptor_set_ptr = &descriptor_set;
    m_vk_binding_value      = binding_value;
}

void ProgramBindingsVK::ArgumentBindingVK::SetDescriptorSet(const vk::DescriptorSet& descriptor_set) noexcept
{
    META_FUNCTION_TASK();
    m_vk_descriptor_set_ptr = &descriptor_set;
}

void ProgramBindingsVK::ArgumentBindingVK::MergeSettings(const ArgumentBindingBase& other)
{
    META_FUNCTION_TASK();
    ArgumentBindingBase::MergeSettings(other);

    const SettingsVK& settings_vk = dynamic_cast<const ArgumentBindingVK&>(other).GetSettingsVK();
    META_CHECK_ARG_EQUAL(m_settings_vk.descriptor_type, settings_vk.descriptor_type);
    m_settings_vk.byte_code_maps.insert(m_settings_vk.byte_code_maps.end(), settings_vk.byte_code_maps.begin(), settings_vk.byte_code_maps.end());
}

bool ProgramBindingsVK::ArgumentBindingVK::SetResourceLocations(const Resource::Locations& resource_locations)
{
    META_FUNCTION_TASK();
    if (!ArgumentBindingBase::SetResourceLocations(resource_locations))
        return false;

    META_CHECK_ARG_NOT_NULL(m_vk_descriptor_set_ptr);

    m_resource_locations_vk.clear();
    m_resource_locations_vk.reserve(resource_locations.size());

    m_vk_write_descriptor_sets.clear();
    m_vk_write_descriptor_sets.reserve(resource_locations.size());

    for(const Resource::Location& resource_location : resource_locations)
    {
        m_resource_locations_vk.emplace_back(resource_location, Resource::Usage::ShaderRead);
        const ResourceLocationVK& resource_location_vk = m_resource_locations_vk.back();

        m_vk_write_descriptor_sets.emplace_back(
            *m_vk_descriptor_set_ptr,
            m_vk_binding_value,
            resource_location.GetSubresourceIndex().GetArrayIndex(),
            1U,
            m_settings_vk.descriptor_type,
            resource_location_vk.GetNativeDescriptorImageInfoPtr(),
            resource_location_vk.GetNativeDescriptorBufferInfoPtr(),
            resource_location_vk.GetNativeBufferViewPtr()
        );
    }

    // Descriptions are updated on GPU during context initialization complete
#ifdef DEFERRED_PROGRAM_BINDINGS_INITIALIZATION
    GetContext().RequestDeferredAction(Context::DeferredAction::CompleteInitialization);
#else
    UpdateDescriptorSetsOnGpu();
#endif
    return true;
}

void ProgramBindingsVK::ArgumentBindingVK::UpdateDescriptorSetsOnGpu()
{
    META_FUNCTION_TASK();
    if (m_vk_write_descriptor_sets.empty())
        return;

    const auto& vulkan_context = dynamic_cast<const IContextVK&>(GetContext());
    vulkan_context.GetDeviceVK().GetNativeDevice().updateDescriptorSets(m_vk_write_descriptor_sets, {});
    m_vk_write_descriptor_sets.clear();
}

ProgramBindingsVK::ProgramBindingsVK(const Ptr<Program>& program_ptr,
                                     const ResourceLocationsByArgument& resource_locations_by_argument,
                                     Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, frame_index)
{
    META_FUNCTION_TASK();
    auto& program = static_cast<ProgramVK&>(GetProgram());
    program.Connect(*this);

    const vk::DescriptorSet& vk_constant_descriptor_set = program.GetConstantDescriptorSet();
    if (vk_constant_descriptor_set)
        m_descriptor_sets.emplace_back(vk_constant_descriptor_set);

    const vk::DescriptorSet& vk_frame_constant_descriptor_set = program.GetFrameConstantDescriptorSet(frame_index);
    if (vk_frame_constant_descriptor_set)
        m_descriptor_sets.emplace_back(vk_frame_constant_descriptor_set);

    vk::DescriptorSet mutable_descriptor_set;
    if (const vk::DescriptorSetLayout& vk_mutable_descriptor_set_layout = program.GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::Mutable);
        vk_mutable_descriptor_set_layout)
    {
        DescriptorManagerVK& descriptor_manager = program.GetContextVK().GetDescriptorManagerVK();
        m_descriptor_sets.emplace_back(descriptor_manager.AllocDescriptorSet(vk_mutable_descriptor_set_layout));
        m_has_mutable_descriptor_set = true;
    }

    // Initialize each argument binding with descriptor set pointer and binding index
    ForEachArgumentBinding([this, &program, &vk_constant_descriptor_set, &vk_frame_constant_descriptor_set]
                           (const Program::Argument& program_argument, ArgumentBindingVK& argument_binding)
    {
        const ArgumentBindingVK::SettingsVK& argument_binding_settings = argument_binding.GetSettingsVK();
        const Program::ArgumentAccessor::Type access_type = argument_binding_settings.argument.GetAccessorType();

        const ProgramVK::DescriptorSetLayoutInfo& layout_info = program.GetDescriptorSetLayoutInfo(access_type);
        const auto layout_argument_it = std::find(layout_info.arguments.begin(), layout_info.arguments.end(), program_argument);
        META_CHECK_ARG_TRUE_DESCR(layout_argument_it != layout_info.arguments.end(), "unable to find argument '{}' in descriptor set layout", program_argument);
        const uint32_t layout_binding_index = static_cast<uint32_t>(std::distance(layout_info.arguments.begin(), layout_argument_it));
        const uint32_t binding_value = layout_info.bindings.at(layout_binding_index).binding;

        switch (access_type)
        {
        case Program::ArgumentAccessor::Type::Constant:
            META_CHECK_ARG_TRUE(!!vk_constant_descriptor_set);
            argument_binding.SetDescriptorSetBinding(vk_constant_descriptor_set, binding_value);
            break;

        case Program::ArgumentAccessor::Type::FrameConstant:
            META_CHECK_ARG_TRUE(!!vk_frame_constant_descriptor_set);
            argument_binding.SetDescriptorSetBinding(vk_frame_constant_descriptor_set, binding_value);
            break;

        case Program::ArgumentAccessor::Type::Mutable:
            META_CHECK_ARG_TRUE(m_has_mutable_descriptor_set);
            argument_binding.SetDescriptorSetBinding(m_descriptor_sets.back(), binding_value);
            break;
        }
    });

    UpdateMutableDescriptorSetName();
    SetResourcesForArguments(resource_locations_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindingsVK::ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings,
                                     const ResourceLocationsByArgument& replace_resource_location_by_argument,
                                     const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, frame_index)
    , m_descriptor_sets(other_program_bindings.m_descriptor_sets)
    , m_has_mutable_descriptor_set(other_program_bindings.m_has_mutable_descriptor_set)
{
    META_FUNCTION_TASK();

    if (m_has_mutable_descriptor_set)
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
            vk::CopyDescriptorSet(other_program_bindings.m_descriptor_sets.back(), {}, {}, copy_mutable_descriptor_set, {}, mutable_desc_set_layout_info.descriptors_count)
        });

        vk::DescriptorSet& vk_mutable_descriptor_set = m_descriptor_sets.back();
        vk_mutable_descriptor_set = copy_mutable_descriptor_set;

        // Update mutable argument bindings with a pointer to the copied descriptor set
        ForEachArgumentBinding([&vk_mutable_descriptor_set](const Program::Argument&, ArgumentBindingVK& argument_binding)
        {
            if (argument_binding.GetSettingsVK().argument.GetAccessorType() != Program::ArgumentAccessor::Type::Mutable)
                return;

            argument_binding.SetDescriptorSet(vk_mutable_descriptor_set);
        });
    }

    UpdateMutableDescriptorSetName();
    SetResourcesForArguments(ReplaceResourceLocations(other_program_bindings.GetArgumentBindings(), replace_resource_location_by_argument));
    VerifyAllArgumentsAreBoundToResources();
}

void ProgramBindingsVK::Initialize()
{
    META_FUNCTION_TASK();
    const ContextBase& context = static_cast<ProgramBase&>(GetProgram()).GetContext();
    context.GetDescriptorManagerVK().AddProgramBindings(*this);
}

void ProgramBindingsVK::CompleteInitialization()
{
    META_FUNCTION_TASK();
    META_LOG("Update descriptor sets on GPU for program bindings '{}'", GetName());

    ForEachArgumentBinding([](const Program::Argument&, ArgumentBindingVK& argument_binding)
    {
        argument_binding.UpdateDescriptorSetsOnGpu();
    });
}

void ProgramBindingsVK::Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    Apply(dynamic_cast<ICommandListVK&>(command_list), command_list.GetCommandQueue(),
          command_list.GetProgramBindingsPtr(), apply_behavior);
}

void ProgramBindingsVK::Apply(ICommandListVK& command_list_vk, CommandQueue& command_queue,
                              const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY(m_descriptor_sets);
    using namespace magic_enum::bitwise_operators;

    Program::ArgumentAccessor::Type apply_access_mask = Program::ArgumentAccessor::Type::Mutable;
    uint32_t first_descriptor_set_layout_index = 0U;
    if (apply_behavior == ApplyBehavior::ConstantOnce && p_applied_program_bindings)
    {
        if (!m_has_mutable_descriptor_set)
            return;

        first_descriptor_set_layout_index = static_cast<uint32_t>(m_descriptor_sets.size() - 1);
    }
    else
    {
        apply_access_mask |= Program::ArgumentAccessor::Type::Constant;
        apply_access_mask |= Program::ArgumentAccessor::Type::FrameConstant;
    }

    // Set resource transition barriers before applying resource bindings
    if (magic_enum::flags::enum_contains(apply_behavior & ApplyBehavior::StateBarriers))
    {
        ProgramBindingsBase::ApplyResourceTransitionBarriers(command_list_vk, apply_access_mask, &command_queue);
    }

    const vk::CommandBuffer&    vk_command_buffer      = command_list_vk.GetNativeCommandBufferDefault();
    const vk::PipelineBindPoint vk_pipeline_bind_point = command_list_vk.GetNativePipelineBindPoint();

    // Bind descriptor sets to pipeline
    auto& program = static_cast<ProgramVK&>(GetProgram());
    vk_command_buffer.bindDescriptorSets(vk_pipeline_bind_point,
                                         program.GetNativePipelineLayout(),
                                         first_descriptor_set_layout_index,
                                         m_descriptor_sets,
                                         {});
}

void ProgramBindingsVK::OnObjectNameChanged(Object&, const std::string&)
{
    META_FUNCTION_TASK();
    UpdateMutableDescriptorSetName();
}

template<typename FuncType> // function void(const Program::Argument&, ArgumentBindingVK&)
void ProgramBindingsVK::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        argument_binding_function(program_argument, static_cast<ArgumentBindingVK&>(*argument_binding_ptr));
    }
}

void ProgramBindingsVK::UpdateMutableDescriptorSetName()
{
    META_FUNCTION_TASK();
    if (!m_has_mutable_descriptor_set)
        return;

    const std::string& program_name = GetProgram().GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(static_cast<ProgramVK&>(GetProgram()).GetContextVK().GetDeviceVK().GetNativeDevice(), m_descriptor_sets.back(),
                        fmt::format("{} Mutable Argument Bindings {}", program_name, GetBindingsIndex()));
}

} // namespace Methane::Graphics
