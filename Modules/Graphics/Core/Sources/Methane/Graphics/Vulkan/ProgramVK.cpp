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

#include "ProgramVK.h"
#include "ShaderVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "UtilsVK.hpp"
#include "ProgramBindingsVK.h"
#include "DescriptorManagerVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <sstream>

namespace Methane::Graphics
{

Ptr<Program> Program::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramVK>(dynamic_cast<const ContextBase&>(context), settings);
}

ProgramVK::ProgramVK(const ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
{
    META_FUNCTION_TASK();
    InitArgumentBindings(settings.argument_accessors);
    InitializeDescriptorSetLayouts();
}

bool ProgramVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!ProgramBase::SetName(name))
        return false;

    UpdatePipelineName();
    UpdateDescriptorSetLayoutNames();
    UpdateConstantDescriptorSetName();
    UpdateFrameConstantDescriptorSetNames();

    return true;
}

const IContextVK& ProgramVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContext());
}

ShaderVK& ProgramVK::GetShaderVK(ShaderType shader_type) const
{
    META_FUNCTION_TASK();
    return static_cast<ShaderVK&>(GetShaderRef(shader_type));
}

std::vector<vk::PipelineShaderStageCreateInfo> ProgramVK::GetNativeShaderStageCreateInfos() const
{
    META_FUNCTION_TASK();
    std::vector<vk::PipelineShaderStageCreateInfo> vk_stage_create_infos;
    for(ShaderType shader_type : GetShaderTypes())
    {
        vk_stage_create_infos.emplace_back(GetShaderVK(shader_type).GetNativeStageCreateInfo());
    }
    return vk_stage_create_infos;
}

vk::PipelineVertexInputStateCreateInfo ProgramVK::GetNativeVertexInputStateCreateInfo() const
{
    META_FUNCTION_TASK();
    auto& vertex_shader = static_cast<ShaderVK&>(GetShaderRef(ShaderType::Vertex));
    return vertex_shader.GetNativeVertexInputStateCreateInfo(*this);
}

const std::vector<vk::DescriptorSetLayout>& ProgramVK::GetNativeDescriptorSetLayouts() const
{
    META_FUNCTION_TASK();
    return m_vk_descriptor_set_layouts;
}

const vk::DescriptorSetLayout& ProgramVK::GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type argument_access_type)
{
    META_FUNCTION_TASK();
    static const vk::DescriptorSetLayout s_empty_layout;
    const DescriptorSetLayoutInfo& layout_info = m_descriptor_set_layout_info_by_access_type[*magic_enum::enum_index(argument_access_type)];
    return layout_info.index_opt ? m_vk_unique_descriptor_set_layouts[*layout_info.index_opt].get() : s_empty_layout;
}

const ProgramVK::DescriptorSetLayoutInfo& ProgramVK::GetDescriptorSetLayoutInfo(Program::ArgumentAccessor::Type argument_access_type)
{
    META_FUNCTION_TASK();
    return m_descriptor_set_layout_info_by_access_type[*magic_enum::enum_index(argument_access_type)];
}

const vk::PipelineLayout& ProgramVK::GetNativePipelineLayout()
{
    META_FUNCTION_TASK();
    if (m_vk_unique_pipeline_layout)
        return m_vk_unique_pipeline_layout.get();

    const std::vector<vk::DescriptorSetLayout>& vk_descriptor_set_layouts = GetNativeDescriptorSetLayouts();
    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();

    m_vk_unique_pipeline_layout = vk_device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, vk_descriptor_set_layouts));
    UpdatePipelineName();

    return m_vk_unique_pipeline_layout.get();
}

const vk::DescriptorSet& ProgramVK::GetConstantDescriptorSet()
{
    META_FUNCTION_TASK();
    if (m_vk_constant_descriptor_set_opt.has_value())
        return m_vk_constant_descriptor_set_opt.value();

    const vk::DescriptorSetLayout& layout = GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::Constant);
    m_vk_constant_descriptor_set_opt = layout
                                     ? GetContextVK().GetDescriptorManagerVK().AllocDescriptorSet(layout)
                                     : vk::DescriptorSet();

    UpdateConstantDescriptorSetName();
    return m_vk_constant_descriptor_set_opt.value();
}

const vk::DescriptorSet& ProgramVK::GetFrameConstantDescriptorSet(Data::Index frame_index)
{
    META_FUNCTION_TASK();
    if (!m_vk_frame_constant_descriptor_sets.empty())
    {
        META_CHECK_ARG_LESS(frame_index, m_vk_frame_constant_descriptor_sets.size());
        return m_vk_frame_constant_descriptor_sets[frame_index];
    }

    const Data::Size frames_count = GetContext().GetType() == IContext::Type::Render
                                  ? dynamic_cast<const RenderContextBase&>(GetContext()).GetSettings().frame_buffers_count
                                  : 1U;
    m_vk_frame_constant_descriptor_sets.resize(frames_count);
    META_CHECK_ARG_LESS(frame_index, frames_count);

    const vk::DescriptorSetLayout& layout = GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type::FrameConstant);
    if (!layout)
        return m_vk_frame_constant_descriptor_sets.at(frame_index);

    DescriptorManagerVK& descriptor_manager = GetContextVK().GetDescriptorManagerVK();
    for(vk::DescriptorSet& frame_descriptor_set : m_vk_frame_constant_descriptor_sets)
    {
        frame_descriptor_set = descriptor_manager.AllocDescriptorSet(layout);
    }

    UpdateFrameConstantDescriptorSetNames();
    return m_vk_frame_constant_descriptor_sets.at(frame_index);
}

void ProgramVK::InitializeDescriptorSetLayouts()
{
    META_FUNCTION_TASK();
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        const auto& vulkan_argument_binding = dynamic_cast<const ProgramBindingsVK::ArgumentBindingVK&>(*argument_binding_ptr);
        const ProgramBindingsVK::ArgumentBindingVK::SettingsVK& vulkan_binding_settings = vulkan_argument_binding.GetSettingsVK();
        const size_t accessor_type_index = magic_enum::enum_index(vulkan_binding_settings.argument.GetAccessorType()).value();

        DescriptorSetLayoutInfo& layout_info = m_descriptor_set_layout_info_by_access_type[accessor_type_index];
        layout_info.descriptors_count += vulkan_binding_settings.resource_count;
        layout_info.arguments.emplace_back(vulkan_binding_settings.argument);
        layout_info.byte_code_maps_for_arguments.emplace_back(vulkan_binding_settings.byte_code_maps);
        layout_info.bindings.emplace_back(
            static_cast<uint32_t>(layout_info.bindings.size()),
            vulkan_binding_settings.descriptor_type,
            vulkan_binding_settings.resource_count,
            ShaderVK::ConvertTypeToStageFlagBits(program_argument.GetShaderType())
        );
    }

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << "Program '" << GetName() << "' with descriptor set layouts:" << std::endl;
#endif

    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();

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
                Data::MutableChunk& spirv_shader_bytecode = GetShaderVK(byte_code_map.shader_type).GetMutableByteCode();
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

void ProgramVK::UpdatePipelineName()
{
    if (!m_vk_unique_pipeline_layout)
        return;

    const std::string& program_name = GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), m_vk_unique_pipeline_layout.get(),
                        fmt::format("{} Pipeline Layout", program_name));
}

void ProgramVK::UpdateDescriptorSetLayoutNames() const
{
    META_FUNCTION_TASK();
    const std::string& program_name = GetName();
    if (program_name.empty())
        return;

    size_t layout_index = 0u;
    for (const vk::DescriptorSetLayout& descriptor_set_layout : m_vk_descriptor_set_layouts)
    {
        Program::ArgumentAccessor::Type access_type = magic_enum::enum_value<Program::ArgumentAccessor::Type>(layout_index);
        SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), descriptor_set_layout,
                            fmt::format("{} {} Arguments Layout", program_name, magic_enum::enum_name(access_type)));
        layout_index++;
    }
}

void ProgramVK::UpdateConstantDescriptorSetName()
{
    META_FUNCTION_TASK();
    if (!m_vk_constant_descriptor_set_opt ||
        !m_vk_constant_descriptor_set_opt.value())
        return;

    const std::string& program_name = GetName();
    if (program_name.empty())
        return;

    SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), m_vk_constant_descriptor_set_opt.value(),
        fmt::format("{} Constant Argument Bindings", program_name));
}

void ProgramVK::UpdateFrameConstantDescriptorSetNames() const
{
    META_FUNCTION_TASK();
    if (m_vk_frame_constant_descriptor_sets.empty())
        return;

    const std::string& program_name = GetName();
    if (program_name.empty())
        return;

    size_t frame_index = 0u;
    for (const vk::DescriptorSet& vk_frame_const_descriptor_set : m_vk_frame_constant_descriptor_sets)
    {
        SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), vk_frame_const_descriptor_set,
            fmt::format("{} Frame {} Constant Argument Bindings", program_name, frame_index));
        frame_index++;
    }
}

} // namespace Methane::Graphics
