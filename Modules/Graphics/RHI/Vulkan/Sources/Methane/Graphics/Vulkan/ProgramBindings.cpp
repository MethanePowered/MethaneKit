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

FILE: Methane/Graphics/Vulkan/ProgramBindings.h
Vulkan implementation of the program bindings interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/ProgramBindings.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/ICommandList.h>
#include <Methane/Graphics/Vulkan/DescriptorManager.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

//#define DYNAMIC_BUFFER_OFFSETS_ENABLED

namespace Methane::Graphics::Vulkan
{

ProgramBindings::PushConstantSetter::PushConstantSetter(Rhi::ProgramArgumentAccessType access_type,
                                                        vk::ShaderStageFlags shader_stages, uint32_t offset,
                                                        Base::RootConstantAccessor& root_const_accessor_ref)
    : access_type(access_type)
    , shader_stages(shader_stages)
    , offset(offset)
    , root_const_accessor_ref(root_const_accessor_ref)
{
}

ProgramBindings::ProgramBindings(Program& program,
                                 const BindingValueByArgument& binding_value_by_argument,
                                 Data::Index frame_index)
    : Base::ProgramBindings(program, frame_index)
{
    META_FUNCTION_TASK();
    program.Connect(*this);

    const vk::DescriptorSet& vk_constant_descriptor_set = program.AcquireConstantDescriptorSet();
    if (vk_constant_descriptor_set)
        m_descriptor_sets.emplace_back(vk_constant_descriptor_set);

    const vk::DescriptorSet& vk_frame_constant_descriptor_set = program.AcquireFrameConstantDescriptorSet(frame_index);
    if (vk_frame_constant_descriptor_set)
        m_descriptor_sets.emplace_back(vk_frame_constant_descriptor_set);

    if (const vk::DescriptorSetLayout& vk_mutable_descriptor_set_layout = program.GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessType::Mutable);
        vk_mutable_descriptor_set_layout)
    {
        DescriptorManager& descriptor_manager = program.GetVulkanContext().GetVulkanDescriptorManager();
        m_descriptor_sets.emplace_back(descriptor_manager.AllocDescriptorSet(vk_mutable_descriptor_set_layout));
        m_has_mutable_descriptor_set = true;
    }

    const auto descriptor_set_selector = [this, &vk_constant_descriptor_set, &vk_frame_constant_descriptor_set]
                                         (const Rhi::ProgramArgumentAccessType access_type) -> const vk::DescriptorSet&
    {
        static const vk::DescriptorSet s_empty_set;
        switch (access_type)
        {
        using enum Rhi::ProgramArgumentAccessType;
        case Constant:
            META_CHECK_TRUE(!!vk_constant_descriptor_set);
            return vk_constant_descriptor_set;

        case FrameConstant:
            META_CHECK_TRUE(!!vk_frame_constant_descriptor_set);
            return vk_frame_constant_descriptor_set;

        case Mutable:
            META_CHECK_TRUE(m_has_mutable_descriptor_set);
            return m_descriptor_sets.back();

        default:
            return s_empty_set;
        }
    };

    // Initialize each argument binding with descriptor set pointer and binding index
    ForEachArgumentBinding([this, &program, &descriptor_set_selector]
                           (const Rhi::ProgramArgument& program_argument, ArgumentBinding& argument_binding)
    {
        const ArgumentBinding::Settings& argument_binding_settings = argument_binding.GetVulkanSettings();
        if (argument_binding_settings.argument.GetValueType() == Rhi::ProgramArgumentValueType::RootConstantValue)
        {
            Base::RootConstantAccessor* root_const_accessor_ptr = argument_binding.GetRootConstantAccessorPtr();
            META_CHECK_NOT_NULL(root_const_accessor_ptr);
            m_push_constant_setters.emplace_back(
                argument_binding_settings.argument.GetAccessorType(),
                argument_binding.GetNativeShaderStageFlags(),
                argument_binding.GetPushConstantsOffset(),
                *root_const_accessor_ptr
            );
        }
        else
        {
            const Rhi::ProgramArgumentAccessType access_type = argument_binding_settings.argument.GetAccessorType();
            const Program::DescriptorSetLayoutInfo& layout_info = program.GetDescriptorSetLayoutInfo(access_type);
            const auto layout_argument_it = std::ranges::find(layout_info.arguments, program_argument);
            META_CHECK_TRUE_DESCR(layout_argument_it != layout_info.arguments.end(), "unable to find argument '{}' in descriptor set layout", static_cast<std::string>(program_argument));
            const auto layout_binding_index = static_cast<uint32_t>(std::distance(layout_info.arguments.begin(), layout_argument_it));
            const uint32_t binding_value = layout_info.bindings.at(layout_binding_index).binding;
            const vk::DescriptorSet& descriptor_set = descriptor_set_selector(access_type);
            argument_binding.SetDescriptorSetBinding(descriptor_set, binding_value);
        }
    });

    UpdateMutableDescriptorSetName();
    SetResourcesForArguments(binding_value_by_argument);
    VerifyAllArgumentsAreBoundToResources();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings,
                                 const BindingValueByArgument& replace_resource_view_by_argument,
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
        const auto& program = static_cast<const Program&>(GetProgram());
        const vk::DescriptorSetLayout& vk_mutable_desc_set_layout = program.GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessType::Mutable);
        META_CHECK_NOT_NULL(vk_mutable_desc_set_layout);
        vk::DescriptorSet copy_mutable_descriptor_set = program.GetVulkanContext().GetVulkanDescriptorManager().AllocDescriptorSet(vk_mutable_desc_set_layout);

        // Copy descriptors from original to new mutable descriptor set
        const vk::Device& vk_device = program.GetVulkanContext().GetVulkanDevice().GetNativeDevice();
        const Program::DescriptorSetLayoutInfo& mutable_desc_set_layout_info = program.GetDescriptorSetLayoutInfo(Rhi::ProgramArgumentAccessType::Mutable);
        vk_device.updateDescriptorSets({}, {
            vk::CopyDescriptorSet(other_program_bindings.m_descriptor_sets.back(), {}, {}, copy_mutable_descriptor_set, {}, mutable_desc_set_layout_info.descriptors_count)
        });

        vk::DescriptorSet& vk_mutable_descriptor_set = m_descriptor_sets.back();
        vk_mutable_descriptor_set = copy_mutable_descriptor_set;

        // Update mutable argument bindings with a pointer to the copied descriptor set
        ForEachArgumentBinding([&vk_mutable_descriptor_set](const Rhi::ProgramArgument&, ArgumentBinding& argument_binding)
        {
            if (argument_binding.GetVulkanSettings().argument.GetAccessorType() != Rhi::ProgramArgumentAccessType::Mutable)
                return;

            argument_binding.SetDescriptorSet(vk_mutable_descriptor_set);
        });
    }

    UpdateMutableDescriptorSetName();
    SetResourcesForArguments(ReplaceBindingValues(other_program_bindings.GetArgumentBindings(), replace_resource_view_by_argument));
    VerifyAllArgumentsAreBoundToResources();
}

Ptr<Rhi::IProgramBindings> ProgramBindings::CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument,
                                                       const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<ProgramBindings>(*this, replace_binding_value_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

void ProgramBindings::SetResourcesForArguments(const BindingValueByArgument& binding_value_by_argument)
{
    META_FUNCTION_TASK();
    Base::ProgramBindings::SetResourcesForArguments(binding_value_by_argument);
    UpdateDynamicDescriptorOffsets();
}

void ProgramBindings::CompleteInitialization()
{
    META_FUNCTION_TASK();
    META_LOG("Update descriptor sets on GPU for program bindings '{}'", GetName());

    ForEachArgumentBinding([](const Rhi::ProgramArgument&, ArgumentBinding& argument_binding)
    {
        argument_binding.UpdateDescriptorSetsOnGpu();
    });
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    Apply(dynamic_cast<ICommandList&>(command_list), command_list.GetCommandQueue(),
          command_list.GetProgramBindingsPtr(), apply_behavior);
}

void ProgramBindings::Apply(ICommandList& command_list_vk, const Rhi::ICommandQueue& command_queue,
                            const Base::ProgramBindings* applied_program_bindings_ptr, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    ReleaseRetainedRootConstantBuffers();

    const auto& program = static_cast<const Program&>(GetProgram());
    const vk::PipelineLayout&   vk_pipeline_layout       = program.GetNativePipelineLayout();
    const vk::CommandBuffer&    vk_command_buffer        = command_list_vk.GetNativeCommandBufferDefault();
    const vk::PipelineBindPoint vk_pipeline_bind_point   = command_list_vk.GetNativePipelineBindPoint();

    // Push constants...
    const bool is_constant_binding_applied = apply_behavior.HasAnyBit(ApplyBehavior::ConstantOnce) && applied_program_bindings_ptr;
    for(const PushConstantSetter& push_constant_setter : m_push_constant_setters)
    {
        if (is_constant_binding_applied &&
            (push_constant_setter.access_type == Rhi::ProgramArgumentAccessType::Constant ||
             push_constant_setter.access_type == Rhi::ProgramArgumentAccessType::FrameConstant))
            continue;

        Base::RootConstantAccessor& root_constant_accessor = push_constant_setter.root_const_accessor_ref.get();
        vk_command_buffer.pushConstants(vk_pipeline_layout,
                                        push_constant_setter.shader_stages,
                                        push_constant_setter.offset,
                                        root_constant_accessor.GetDataSize(),
                                        root_constant_accessor.GetDataPtr());
    }

    // Bind descriptor sets...
    if (m_descriptor_sets.empty())
        return;

    Rhi::ProgramArgumentAccessMask apply_access;
    apply_access.SetBitOn(Rhi::ProgramArgumentAccessType::Mutable);
    uint32_t first_descriptor_set_layout_index = 0U;

    if (is_constant_binding_applied)
    {
        if (!m_has_mutable_descriptor_set)
            return;

        first_descriptor_set_layout_index = static_cast<uint32_t>(m_descriptor_sets.size() - 1);
    }
    else
    {
        apply_access.SetBitOn(Rhi::ProgramArgumentAccessType::Constant);
        apply_access.SetBitOn(Rhi::ProgramArgumentAccessType::FrameConstant);
    }

    // Set resource transition barriers before applying resource bindings
    if (apply_behavior.HasAnyBit(ApplyBehavior::StateBarriers))
    {
        Base::ProgramBindings::ApplyResourceTransitionBarriers(command_list_vk, apply_access, &command_queue);
    }

    const uint32_t first_dynamic_offset_index = m_dynamic_offset_index_by_set_index[first_descriptor_set_layout_index];

    // Bind descriptor sets to pipeline
    vk_command_buffer.bindDescriptorSets(vk_pipeline_bind_point,
                                         vk_pipeline_layout,
                                         first_descriptor_set_layout_index,
                                         static_cast<uint32_t>(m_descriptor_sets.size() - first_descriptor_set_layout_index),
                                         m_descriptor_sets.data() + first_descriptor_set_layout_index,
                                         static_cast<uint32_t>(m_dynamic_offsets.size() - first_dynamic_offset_index),
                                         m_dynamic_offsets.data() + first_dynamic_offset_index);
}

void ProgramBindings::OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding& argument_binding,
                                                                   const Rhi::ResourceViews& old_resource_views,
                                                                   const Rhi::ResourceViews& new_resource_views)
{
    META_FUNCTION_TASK();
    Base::ProgramBindings::OnProgramArgumentBindingResourceViewsChanged(argument_binding, old_resource_views, new_resource_views);
    UpdateDynamicDescriptorOffsets();
}

void ProgramBindings::OnObjectNameChanged(IObject&, const std::string&)
{
    META_FUNCTION_TASK();
    UpdateMutableDescriptorSetName();
}

template<typename FuncType> // function void(const Rhi::ProgramArgument&, ArgumentBinding&)
void ProgramBindings::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_NOT_NULL(argument_binding_ptr);
        argument_binding_function(program_argument, static_cast<ArgumentBinding&>(*argument_binding_ptr));
    }
}

void ProgramBindings::UpdateDynamicDescriptorOffsets()
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<const Program&>(GetProgram());
    std::vector<std::vector<uint32_t>> dynamic_offsets_by_set_index;
    dynamic_offsets_by_set_index.resize(m_descriptor_sets.size());

    ForEachArgumentBinding([&program, &dynamic_offsets_by_set_index]
                           (const Rhi::ProgramArgument&, const ArgumentBinding& argument_binding)
        {
            const Rhi::ProgramArgumentAccessor& program_argument_accessor = argument_binding.GetSettings().argument;
            if (!program_argument_accessor.IsAddressable() ||
                 program_argument_accessor.IsRootConstantValue())
                return;

            const Program::DescriptorSetLayoutInfo& layout_info = program.GetDescriptorSetLayoutInfo(program_argument_accessor.GetAccessorType());
            META_CHECK_TRUE(layout_info.index_opt.has_value());
            META_CHECK_LESS(*layout_info.index_opt, dynamic_offsets_by_set_index.size());
            std::vector<uint32_t>& dynamic_offsets = dynamic_offsets_by_set_index[*layout_info.index_opt];
            dynamic_offsets.clear();

            const Rhi::ResourceViews& resource_views = argument_binding.GetResourceViews();
#ifdef DYNAMIC_BUFFER_OFFSETS_ENABLED
            std::ranges::transform(resource_views, std::back_inserter(dynamic_offsets),
                           [](const Rhi::IResource::View& resource_view)
                           { return resource_view.GetOffset(); });
#else
            dynamic_offsets.resize(resource_views.size(), 0U);
#endif
        });

    m_dynamic_offsets.clear();
    m_dynamic_offset_index_by_set_index.clear();
    for (const std::vector<uint32_t>& dynamic_offsets : dynamic_offsets_by_set_index)
    {
        m_dynamic_offset_index_by_set_index.emplace_back(static_cast<uint32_t>(m_dynamic_offsets.size()));
        m_dynamic_offsets.insert(m_dynamic_offsets.end(), dynamic_offsets.begin(), dynamic_offsets.end());
    }
}

void ProgramBindings::UpdateMutableDescriptorSetName()
{
    META_FUNCTION_TASK();
    if (!m_has_mutable_descriptor_set)
        return;

    const std::string_view program_name = GetProgram().GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(static_cast<Program&>(GetProgram()).GetVulkanContext().GetVulkanDevice().GetNativeDevice(), m_descriptor_sets.back(),
                        fmt::format("{} Mutable Argument Bindings {}", program_name, GetBindingsIndex()));
}

} // namespace Methane::Graphics::Vulkan
