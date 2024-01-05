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

#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/ProgramBindings.h>
#include <Methane/Graphics/Vulkan/Shader.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/Utils.hpp>
#include <Methane/Graphics/Vulkan/ProgramBindings.h>
#include <Methane/Graphics/Vulkan/DescriptorManager.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <sstream>

namespace Methane::Graphics::Vulkan
{

Program::Program(const Base::Context& context, const Settings& settings)
    : Base::Program(context, settings)
    , m_vk_context(dynamic_cast<const IContext&>(context))
{
    META_FUNCTION_TASK();
    InitArgumentBindings(settings.argument_accessors);
    InitializeDescriptorSetLayouts();
}

Ptr<Rhi::IProgramBindings> Program::CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<Vulkan::ProgramBindings>(*this, resource_views_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

bool Program::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::Program::SetName(name))
        return false;

    UpdatePipelineName();
    UpdateDescriptorSetLayoutNames();
    UpdateConstantDescriptorSetName();
    UpdateFrameConstantDescriptorSetNames();

    return true;
}

Shader& Program::GetVulkanShader(Rhi::ShaderType shader_type) const
{
    META_FUNCTION_TASK();
    return static_cast<Shader&>(GetShaderRef(shader_type));
}

std::vector<vk::PipelineShaderStageCreateInfo> Program::GetNativeShaderStageCreateInfos() const
{
    META_FUNCTION_TASK();
    std::vector<vk::PipelineShaderStageCreateInfo> vk_stage_create_infos;
    for(Rhi::ShaderType shader_type : GetShaderTypes())
    {
        vk_stage_create_infos.emplace_back(GetVulkanShader(shader_type).GetNativeStageCreateInfo());
    }
    return vk_stage_create_infos;
}

vk::PipelineVertexInputStateCreateInfo Program::GetNativeVertexInputStateCreateInfo() const
{
    META_FUNCTION_TASK();
    auto& vertex_shader = static_cast<Shader&>(GetShaderRef(Rhi::ShaderType::Vertex));
    return vertex_shader.GetNativeVertexInputStateCreateInfo(*this);
}

const std::vector<vk::DescriptorSetLayout>& Program::GetNativeDescriptorSetLayouts() const
{
    META_FUNCTION_TASK();
    return m_vk_descriptor_set_layouts;
}

const vk::DescriptorSetLayout& Program::GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessType argument_access_type) const
{
    META_FUNCTION_TASK();
    static const vk::DescriptorSetLayout s_empty_layout;
    const DescriptorSetLayoutInfo& layout_info = m_descriptor_set_layout_info_by_access_type[*magic_enum::enum_index(argument_access_type)];
    return layout_info.index_opt ? m_vk_unique_descriptor_set_layouts[*layout_info.index_opt].get() : s_empty_layout;
}

const Program::DescriptorSetLayoutInfo& Program::GetDescriptorSetLayoutInfo(Rhi::ProgramArgumentAccessType argument_access_type) const
{
    META_FUNCTION_TASK();
    return m_descriptor_set_layout_info_by_access_type[*magic_enum::enum_index(argument_access_type)];
}

const vk::PipelineLayout& Program::GetNativePipelineLayout() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_vk_unique_pipeline_layout);
    return m_vk_unique_pipeline_layout.get();
}

const vk::PipelineLayout& Program::AcquireNativePipelineLayout()
{
    META_FUNCTION_TASK();
    std::lock_guard lock(m_mutex);

    if (m_vk_unique_pipeline_layout)
        return m_vk_unique_pipeline_layout.get();

    const std::vector<vk::DescriptorSetLayout>& vk_descriptor_set_layouts = GetNativeDescriptorSetLayouts();
    const vk::Device& vk_device = GetVulkanContext().GetVulkanDevice().GetNativeDevice();

    m_vk_unique_pipeline_layout = vk_device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, vk_descriptor_set_layouts));
    UpdatePipelineName();

    return m_vk_unique_pipeline_layout.get();
}

const vk::DescriptorSet& Program::AcquireConstantDescriptorSet()
{
    META_FUNCTION_TASK();
    std::lock_guard lock(m_mutex);

    if (m_vk_constant_descriptor_set_opt.has_value())
        return m_vk_constant_descriptor_set_opt.value();

    const vk::DescriptorSetLayout& layout = GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessType::Constant);
    m_vk_constant_descriptor_set_opt = layout
                                     ? GetVulkanContext().GetVulkanDescriptorManager().AllocDescriptorSet(layout)
                                     : vk::DescriptorSet();

    UpdateConstantDescriptorSetName();
    return m_vk_constant_descriptor_set_opt.value();
}

const vk::DescriptorSet& Program::AcquireFrameConstantDescriptorSet(Data::Index frame_index)
{
    META_FUNCTION_TASK();
    std::lock_guard lock(m_mutex);

    if (!m_vk_frame_constant_descriptor_sets.empty())
    {
        META_CHECK_ARG_LESS(frame_index, m_vk_frame_constant_descriptor_sets.size());
        return m_vk_frame_constant_descriptor_sets[frame_index];
    }

    const Data::Size frames_count = GetContext().GetType() == Rhi::IContext::Type::Render
                                  ? dynamic_cast<const Base::RenderContext&>(GetContext()).GetSettings().frame_buffers_count
                                  : 1U;
    m_vk_frame_constant_descriptor_sets.resize(frames_count);
    META_CHECK_ARG_LESS(frame_index, frames_count);

    const vk::DescriptorSetLayout& layout = GetNativeDescriptorSetLayout(Rhi::ProgramArgumentAccessType::FrameConstant);
    if (!layout)
        return m_vk_frame_constant_descriptor_sets.at(frame_index);

    DescriptorManager& descriptor_manager = GetVulkanContext().GetVulkanDescriptorManager();
    for(vk::DescriptorSet& frame_descriptor_set : m_vk_frame_constant_descriptor_sets)
    {
        frame_descriptor_set = descriptor_manager.AllocDescriptorSet(layout);
    }

    UpdateFrameConstantDescriptorSetNames();
    return m_vk_frame_constant_descriptor_sets.at(frame_index);
}

void Program::InitializeDescriptorSetLayouts()
{
    META_FUNCTION_TASK();
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        const auto& vulkan_argument_binding = dynamic_cast<const ProgramBindings::ArgumentBinding&>(*argument_binding_ptr);
        const ProgramBindings::ArgumentBinding::Settings& vulkan_binding_settings = vulkan_argument_binding.GetVulkanSettings();
        const size_t accessor_type_index = magic_enum::enum_index(vulkan_binding_settings.argument.GetAccessorType()).value();

        DescriptorSetLayoutInfo& layout_info = m_descriptor_set_layout_info_by_access_type[accessor_type_index];
        layout_info.descriptors_count += vulkan_binding_settings.resource_count;
        layout_info.arguments.emplace_back(vulkan_binding_settings.argument);
        layout_info.byte_code_maps_for_arguments.emplace_back(vulkan_binding_settings.byte_code_maps);
        layout_info.bindings.emplace_back(
            static_cast<uint32_t>(layout_info.bindings.size()),
            vulkan_binding_settings.descriptor_type,
            vulkan_binding_settings.resource_count,
            Shader::ConvertTypeToStageFlagBits(program_argument.GetShaderType())
        );
    }

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << "Program '" << GetName() << "' with descriptor set layouts:" << std::endl;
#endif

    const vk::Device& vk_device = GetVulkanContext().GetVulkanDevice().GetNativeDevice();

    m_vk_unique_descriptor_set_layouts.clear();
    for(DescriptorSetLayoutInfo& layout_info : m_descriptor_set_layout_info_by_access_type)
    {
        if (layout_info.bindings.empty())
            continue;

        layout_info.index_opt = static_cast<uint32_t>(m_vk_unique_descriptor_set_layouts.size());

#ifdef METHANE_LOGGING_ENABLED
        log_ss << "  - Descriptor set layout " << *layout_info.index_opt << ":" << std::endl;
#endif
        
        for(const vk::DescriptorSetLayoutBinding& layout_binding : layout_info.bindings)
        {
            // Patch shaders SPIRV byte code with remapped binding and descriptor set decorations
            const ByteCodeMaps& byte_code_maps = layout_info.byte_code_maps_for_arguments.at(layout_binding.binding);
            for(const ByteCodeMap& byte_code_map : byte_code_maps)
            {
                Data::MutableChunk& spirv_shader_bytecode = GetVulkanShader(byte_code_map.shader_type).GetMutableByteCode();
                spirv_shader_bytecode.PatchData(byte_code_map.descriptor_set_offset, *layout_info.index_opt);
                spirv_shader_bytecode.PatchData(byte_code_map.binding_offset, layout_binding.binding);
            }

#ifdef METHANE_LOGGING_ENABLED
            log_ss << "    - Binding "      << *layout_info.index_opt << "." << layout_binding.binding
                   << " of "                << vk::to_string(layout_binding.descriptorType)
                   << " descriptors count " << layout_binding.descriptorCount
                   << " for argument '"     << layout_info.arguments.at(layout_binding.binding).GetName()
                   << "' on stage "         << vk::to_string(layout_binding.stageFlags)
                   << ";" << std::endl;
#endif
        }

        m_vk_unique_descriptor_set_layouts.emplace_back(
            vk_device.createDescriptorSetLayoutUnique(
                vk::DescriptorSetLayoutCreateInfo({}, layout_info.bindings)
            ));
    }

    META_LOG("{}", log_ss.str());

    m_vk_descriptor_set_layouts = vk::uniqueToRaw(m_vk_unique_descriptor_set_layouts);

    UpdateDescriptorSetLayoutNames();
}

void Program::UpdatePipelineName()
{
    if (!m_vk_unique_pipeline_layout)
        return;

    const std::string_view program_name = GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(GetVulkanContext().GetVulkanDevice().GetNativeDevice(), m_vk_unique_pipeline_layout.get(),
                        fmt::format("{} Pipeline Layout", program_name));
}

void Program::UpdateDescriptorSetLayoutNames() const
{
    META_FUNCTION_TASK();
    const std::string_view program_name = GetName();
    if (program_name.empty())
        return;

    size_t layout_index = 0u;
    for (const vk::DescriptorSetLayout& descriptor_set_layout : m_vk_descriptor_set_layouts)
    {
        Rhi::ProgramArgumentAccessType access_type = magic_enum::enum_value<Rhi::ProgramArgumentAccessType>(layout_index);
        SetVulkanObjectName(GetVulkanContext().GetVulkanDevice().GetNativeDevice(), descriptor_set_layout,
                            fmt::format("{} {} Arguments Layout", program_name, magic_enum::enum_name(access_type)));
        layout_index++;
    }
}

void Program::UpdateConstantDescriptorSetName()
{
    META_FUNCTION_TASK();
    if (!m_vk_constant_descriptor_set_opt ||
        !m_vk_constant_descriptor_set_opt.value())
        return;

    const std::string_view program_name = GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(GetVulkanContext().GetVulkanDevice().GetNativeDevice(), m_vk_constant_descriptor_set_opt.value(),
        fmt::format("{} Constant Argument Bindings", program_name));
}

void Program::UpdateFrameConstantDescriptorSetNames() const
{
    META_FUNCTION_TASK();
    if (m_vk_frame_constant_descriptor_sets.empty())
        return;

    const std::string_view program_name = GetName();
    if (program_name.empty())
        return;

    size_t frame_index = 0u;
    for (const vk::DescriptorSet& vk_frame_const_descriptor_set : m_vk_frame_constant_descriptor_sets)
    {
        SetVulkanObjectName(GetVulkanContext().GetVulkanDevice().GetNativeDevice(), vk_frame_const_descriptor_set,
            fmt::format("{} Frame {} Constant Argument Bindings", program_name, frame_index));
        frame_index++;
    }
}

} // namespace Methane::Graphics::Vulkan
