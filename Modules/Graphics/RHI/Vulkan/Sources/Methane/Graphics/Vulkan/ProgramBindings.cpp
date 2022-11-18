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

FILE: Methane/Graphics/Vulkan/Program.h
Vulkan implementation of the program interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ProgramBindings.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/CommandList.h>
#include <Methane/Graphics/Vulkan/DescriptorManager.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <algorithm>

namespace Methane::Graphics::Rhi
{

Ptr<IProgramBindings> Rhi::IProgramBindings::Create(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<Vulkan::ProgramBindings>(program_ptr, resource_views_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

Ptr<IProgramBindings> Rhi::IProgramBindings::CreateCopy(const Rhi::IProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<Vulkan::ProgramBindings>(static_cast<const Vulkan::ProgramBindings&>(other_program_bindings), replace_resource_view_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

ProgramBindings::ProgramBindings(const Ptr<Rhi::IProgram>& program_ptr,
                                 const ResourceViewsByArgument& resource_views_by_argument,
                                 Data::Index frame_index)
    : Base::ProgramBindings(program_ptr, frame_index)
{
    META_FUNCTION_TASK();
    auto& program = static_cast<Program&>(GetProgram());
    program.Connect(*this);

    const vk::DescriptorSet& vk_constant_descriptor_set = program.GetConstantDescriptorSet();
    if (vk_constant_descriptor_set)
        m_descriptor_sets.emplace_back(vk_constant_descriptor_set);

    const vk::DescriptorSet& vk_frame_constant_descriptor_set = program.GetFrameConstantDescriptorSet(frame_index);
    if (vk_frame_constant_descriptor_set)
        m_descriptor_sets.emplace_back(vk_frame_constant_descriptor_set);

    if (const vk::DescriptorSetLayout& vk_mutable_descriptor_set_layout = program.GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessor::Type::Mutable);
        vk_mutable_descriptor_set_layout)
    {
        DescriptorManager& descriptor_manager = program.GetVulkanContext().GetVulkanDescriptorManager();
        m_descriptor_sets.emplace_back(descriptor_manager.AllocDescriptorSet(vk_mutable_descriptor_set_layout));
        m_has_mutable_descriptor_set = true;
    }

    const auto destrictor_set_selector = [this, &vk_constant_descriptor_set, &vk_frame_constant_descriptor_set]
                                         (const Rhi::ProgramArgumentAccessor::Type access_type) -> const vk::DescriptorSet&
    {
        static const vk::DescriptorSet s_empty_set;
        switch (access_type)
        {
        case Rhi::ProgramArgumentAccessor::Type::Constant:
            META_CHECK_ARG_TRUE(!!vk_constant_descriptor_set);
            return vk_constant_descriptor_set;

        case Rhi::ProgramArgumentAccessor::Type::FrameConstant:
            META_CHECK_ARG_TRUE(!!vk_frame_constant_descriptor_set);
            return vk_frame_constant_descriptor_set;

        case Rhi::ProgramArgumentAccessor::Type::Mutable:
            META_CHECK_ARG_TRUE(m_has_mutable_descriptor_set);
            return m_descriptor_sets.back();

        default:
            return s_empty_set;
        }
    };

    // Initialize each argument binding with descriptor set pointer and binding index
    ForEachArgumentBinding([&program, &destrictor_set_selector](const Rhi::IProgram::Argument& program_argument, ArgumentBinding& argument_binding)
    {
        const ArgumentBinding::Settings& argument_binding_settings = argument_binding.GetVulkanSettings();
        const Rhi::ProgramArgumentAccessor::Type access_type = argument_binding_settings.argument.GetAccessorType();
        const Program::DescriptorSetLayoutInfo& layout_info = program.GetDescriptorSetLayoutInfo(access_type);
        const auto layout_argument_it = std::find(layout_info.arguments.begin(), layout_info.arguments.end(), program_argument);
        META_CHECK_ARG_TRUE_DESCR(layout_argument_it != layout_info.arguments.end(), "unable to find argument '{}' in descriptor set layout", static_cast<std::string>(program_argument));
        const auto layout_binding_index = static_cast<uint32_t>(std::distance(layout_info.arguments.begin(), layout_argument_it));
        const uint32_t binding_value = layout_info.bindings.at(layout_binding_index).binding;
        const vk::DescriptorSet& descriptor_set = destrictor_set_selector(access_type);

        argument_binding.SetDescriptorSetBinding(descriptor_set, binding_value);
    });

    UpdateMutableDescriptorSetName();
    SetResourcesForArguments(resource_views_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings,
                                     const ResourceViewsByArgument& replace_resource_view_by_argument,
                                     const Opt<Data::Index>& frame_index)
    : Base::ProgramBindings(other_program_bindings, frame_index)
    , m_descriptor_sets(other_program_bindings.m_descriptor_sets)
    , m_has_mutable_descriptor_set(other_program_bindings.m_has_mutable_descriptor_set)
    , m_dynamic_offsets(other_program_bindings.m_dynamic_offsets)
    , m_dynamic_offset_index_by_set_index(other_program_bindings.m_dynamic_offset_index_by_set_index)
{
    META_FUNCTION_TASK();

    if (m_has_mutable_descriptor_set)
    {
        // Allocate new mutable descriptor set
        auto& program = static_cast<Program&>(GetProgram());
        const vk::DescriptorSetLayout& vk_mutable_desc_set_layout = program.GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessor::Type::Mutable);
        META_CHECK_ARG_NOT_NULL(vk_mutable_desc_set_layout);
        vk::DescriptorSet copy_mutable_descriptor_set = program.GetVulkanContext().GetVulkanDescriptorManager().AllocDescriptorSet(vk_mutable_desc_set_layout);

        // Copy descriptors from original to new mutable descriptor set
        const vk::Device& vk_device = program.GetVulkanContext().GetVulkanDevice().GetNativeDevice();
        const Program::DescriptorSetLayoutInfo& mutable_desc_set_layout_info = program.GetDescriptorSetLayoutInfo(Rhi::ProgramArgumentAccessor::Type::Mutable);
        vk_device.updateDescriptorSets({}, {
            vk::CopyDescriptorSet(other_program_bindings.m_descriptor_sets.back(), {}, {}, copy_mutable_descriptor_set, {}, mutable_desc_set_layout_info.descriptors_count)
        });

        vk::DescriptorSet& vk_mutable_descriptor_set = m_descriptor_sets.back();
        vk_mutable_descriptor_set = copy_mutable_descriptor_set;

        // Update mutable argument bindings with a pointer to the copied descriptor set
        ForEachArgumentBinding([&vk_mutable_descriptor_set](const Rhi::IProgram::Argument&, ArgumentBinding& argument_binding)
        {
            if (argument_binding.GetVulkanSettings().argument.GetAccessorType() != Rhi::ProgramArgumentAccessor::Type::Mutable)
                return;

            argument_binding.SetDescriptorSet(vk_mutable_descriptor_set);
        });
    }

    UpdateMutableDescriptorSetName();
    SetResourcesForArguments(ReplaceResourceViews(other_program_bindings.GetArgumentBindings(), replace_resource_view_by_argument));
    VerifyAllArgumentsAreBoundToResources();
}

void ProgramBindings::SetResourcesForArguments(const ResourceViewsByArgument& resource_views_by_argument)
{
    META_FUNCTION_TASK();
    Base::ProgramBindings::SetResourcesForArguments(resource_views_by_argument);

    auto& program = static_cast<Program&>(GetProgram());
    const Rhi::ProgramArgumentAccessors& program_argument_accessors = program.GetSettings().argument_accessors;
    std::vector<std::vector<uint32_t>> dynamic_offsets_by_set_index;
    dynamic_offsets_by_set_index.resize(m_descriptor_sets.size());

    ForEachArgumentBinding([&program, &program_argument_accessors, &dynamic_offsets_by_set_index]
                           (const Rhi::IProgram::Argument& program_argument, const ArgumentBinding& argument_binding)
        {
            const auto program_accessor_it = Rhi::IProgram::FindArgumentAccessor(program_argument_accessors, program_argument);
            META_CHECK_ARG(program_argument, program_accessor_it != program_argument_accessors.end());
            const Rhi::ProgramArgumentAccessor& program_argument_accessor = *program_accessor_it;
            if (!program_argument_accessor.IsAddressable())
                return;

            const Program::DescriptorSetLayoutInfo& layout_info = program.GetDescriptorSetLayoutInfo(program_argument_accessor.GetAccessorType());
            META_CHECK_ARG_TRUE(layout_info.index_opt.has_value());
            META_CHECK_ARG_LESS(*layout_info.index_opt, dynamic_offsets_by_set_index.size());
            std::vector<uint32_t>& dynamic_offsets = dynamic_offsets_by_set_index[*layout_info.index_opt];
            dynamic_offsets.clear();

            const Rhi::ResourceViews& resource_views = argument_binding.GetResourceViews();
            std::transform(resource_views.begin(), resource_views.end(), std::back_inserter(dynamic_offsets),
                           [](const Rhi::IResource::View& resource_view)
                           { return resource_view.GetOffset(); });
        });

    m_dynamic_offsets.clear();
    m_dynamic_offset_index_by_set_index.clear();
    for (const std::vector<uint32_t>& dynamic_offsets : dynamic_offsets_by_set_index)
    {
        m_dynamic_offset_index_by_set_index.emplace_back(static_cast<uint32_t>(m_dynamic_offsets.size()));
        m_dynamic_offsets.insert(m_dynamic_offsets.end(), dynamic_offsets.begin(), dynamic_offsets.end());
    }
}

void ProgramBindings::Initialize()
{
    META_FUNCTION_TASK();
    static_cast<Program&>(GetProgram()).GetVulkanContext().GetVulkanDescriptorManager().AddProgramBindings(*this);
}

void ProgramBindings::CompleteInitialization()
{
    META_FUNCTION_TASK();
    META_LOG("Update descriptor sets on GPU for program bindings '{}'", GetName());

    ForEachArgumentBinding([](const Rhi::IProgram::Argument&, ArgumentBinding& argument_binding)
    {
        argument_binding.UpdateDescriptorSetsOnGpu();
    });
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    Apply(dynamic_cast<ICommandListVk&>(command_list), command_list.GetCommandQueue(),
          command_list.GetProgramBindingsPtr(), apply_behavior);
}

void ProgramBindings::Apply(ICommandListVk& command_list_vk, const Rhi::ICommandQueue& command_queue,
                              const Base::ProgramBindings* p_applied_program_bindings, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY(m_descriptor_sets);
    using namespace magic_enum::bitwise_operators;

    Rhi::ProgramArgumentAccessor::Type apply_access_mask = Rhi::ProgramArgumentAccessor::Type::Mutable;
    uint32_t first_descriptor_set_layout_index = 0U;
    if (apply_behavior == ApplyBehavior::ConstantOnce && p_applied_program_bindings)
    {
        if (!m_has_mutable_descriptor_set)
            return;

        first_descriptor_set_layout_index = static_cast<uint32_t>(m_descriptor_sets.size() - 1);
    }
    else
    {
        apply_access_mask |= Rhi::ProgramArgumentAccessor::Type::Constant;
        apply_access_mask |= Rhi::ProgramArgumentAccessor::Type::FrameConstant;
    }

    // Set resource transition barriers before applying resource bindings
    if (static_cast<bool>(apply_behavior & ApplyBehavior::StateBarriers))
    {
        Base::ProgramBindings::ApplyResourceTransitionBarriers(command_list_vk, apply_access_mask, &command_queue);
    }

    const vk::CommandBuffer&    vk_command_buffer      = command_list_vk.GetNativeCommandBufferDefault();
    const vk::PipelineBindPoint vk_pipeline_bind_point = command_list_vk.GetNativePipelineBindPoint();
    const uint32_t first_dynamic_offset_index = m_dynamic_offset_index_by_set_index[first_descriptor_set_layout_index];

    // Bind descriptor sets to pipeline
    auto& program = static_cast<Program&>(GetProgram());
    vk_command_buffer.bindDescriptorSets(vk_pipeline_bind_point,
                                         program.GetNativePipelineLayout(),
                                         first_descriptor_set_layout_index,
                                         static_cast<uint32_t>(m_descriptor_sets.size() - first_descriptor_set_layout_index),
                                         m_descriptor_sets.data() + first_descriptor_set_layout_index,
                                         static_cast<uint32_t>(m_dynamic_offsets.size() - first_dynamic_offset_index),
                                         m_dynamic_offsets.data() + first_dynamic_offset_index);
}

void ProgramBindings::OnObjectNameChanged(IObject&, const std::string&)
{
    META_FUNCTION_TASK();
    UpdateMutableDescriptorSetName();
}

template<typename FuncType> // function void(const Rhi::IProgram::Argument&, ArgumentBinding&)
void ProgramBindings::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        argument_binding_function(program_argument, static_cast<ArgumentBinding&>(*argument_binding_ptr));
    }
}

void ProgramBindings::UpdateMutableDescriptorSetName()
{
    META_FUNCTION_TASK();
    if (!m_has_mutable_descriptor_set)
        return;

    const std::string& program_name = GetProgram().GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(static_cast<Program&>(GetProgram()).GetVulkanContext().GetVulkanDevice().GetNativeDevice(), m_descriptor_sets.back(),
                        fmt::format("{} Mutable Argument Bindings {}", program_name, GetBindingsIndex()));
}

} // namespace Methane::Graphics::Vulkan