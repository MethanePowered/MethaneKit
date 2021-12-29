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

#include <sstream>

namespace Methane::Graphics
{

Ptr<Program> Program::Create(const Context& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramVK>(dynamic_cast<const ContextBase&>(context), settings);
}

ProgramVK::ProgramVK(const ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
{
    META_FUNCTION_TASK();
    InitArgumentBindings(settings.argument_accessors);
}

void ProgramVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

    ProgramBase::SetName(name);

    const std::string pipeline_name = fmt::format("{} Pipeline Layout", name);
    SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), GetNativePipelineLayout(), pipeline_name.c_str());
}

const IContextVK& ProgramVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContext());
}

ShaderVK& ProgramVK::GetShaderVK(Shader::Type shader_type) noexcept
{
    META_FUNCTION_TASK();
    return static_cast<ShaderVK&>(GetShaderRef(shader_type));
}

std::vector<vk::PipelineShaderStageCreateInfo> ProgramVK::GetNativeShaderStageCreateInfos() const
{
    META_FUNCTION_TASK();
    std::vector<vk::PipelineShaderStageCreateInfo> vk_stage_create_infos;
    for(Shader::Type shader_type : GetShaderTypes())
    {
        const auto& shader = static_cast<const ShaderVK&>(GetShaderRef(shader_type));
        vk_stage_create_infos.emplace_back(shader.GetNativeStageCreateInfo());
    }
    return vk_stage_create_infos;
}

vk::PipelineVertexInputStateCreateInfo ProgramVK::GetNativeVertexInputStateCreateInfo() const
{
    META_FUNCTION_TASK();
    auto& vertex_shader = static_cast<ShaderVK&>(GetShaderRef(Shader::Type::Vertex));
    return vertex_shader.GetNativeVertexInputStateCreateInfo(*this);
}

const std::vector<vk::DescriptorSetLayout>& ProgramVK::GetNativeDescriptorSetLayouts()
{
    META_FUNCTION_TASK();
    if (!m_vk_descriptor_set_layouts_opt)
        InitializeDescriptorSetLayouts();

    return *m_vk_descriptor_set_layouts_opt;
}

const vk::DescriptorSetLayout& ProgramVK::GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type argument_access_type)
{
    META_FUNCTION_TASK();
    if (!m_vk_descriptor_set_layouts_opt)
        InitializeDescriptorSetLayouts();

    static const vk::DescriptorSetLayout s_empty_layout;
    const DescriptorSetLayoutInfo& layout_info = m_descriptor_set_layout_info_by_access_type[*magic_enum::enum_index(argument_access_type)];
    return layout_info.index_opt ? m_vk_unique_descriptor_set_layouts[*layout_info.index_opt].get() : s_empty_layout;
}

const ProgramVK::DescriptorSetLayoutInfo& ProgramVK::GetDescriptorSetLayoutInfo(Program::ArgumentAccessor::Type argument_access_type)
{
    META_FUNCTION_TASK();
    if (!m_vk_descriptor_set_layouts_opt)
        InitializeDescriptorSetLayouts();

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

    const Data::Size frames_count = GetContext().GetType() == Context::Type::Render
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
        layout_info.shader_descriptor_sets.emplace_back(vulkan_binding_settings.descriptor_set);
        layout_info.bindings.emplace_back(
            vulkan_binding_settings.binding,
            vulkan_binding_settings.descriptor_type,
            vulkan_binding_settings.resource_count,
            ShaderVK::ConvertTypeToStageFlagBits(program_argument.GetShaderType())
        );
    }

    std::stringstream log_ss;
    log_ss << "Program '" << GetName() << "' with descriptor set layouts:" << std::endl;

    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();
    bool has_invalid_descriptor_sets = false;

    m_vk_unique_descriptor_set_layouts.clear();
    for(DescriptorSetLayoutInfo& layout_info : m_descriptor_set_layout_info_by_access_type)
    {
        if (layout_info.bindings.empty())
            continue;

        m_vk_unique_descriptor_set_layouts.emplace_back(
            vk_device.createDescriptorSetLayoutUnique(
                vk::DescriptorSetLayoutCreateInfo({}, layout_info.bindings)
            ));

        layout_info.index_opt = static_cast<uint32_t>(m_vk_unique_descriptor_set_layouts.size() - 1);

        log_ss << "  - Descriptor set layout " << *layout_info.index_opt << ":" << std::endl;
        uint32_t layout_binding_index = 0;

        for(const vk::DescriptorSetLayoutBinding& layout_binding : layout_info.bindings)
        {
            log_ss << "    - Binding "      << layout_binding.binding
                   << " descriptors count " << layout_binding.descriptorCount
                   << " of type "           << vk::to_string(layout_binding.descriptorType)
                   << " for argument '"     << layout_info.arguments[layout_binding_index].GetName()
                   << "' on stage "         << vk::to_string(layout_binding.stageFlags);

            const uint32_t shader_descriptor_set = layout_info.shader_descriptor_sets[layout_binding_index];
            if (shader_descriptor_set != *layout_info.index_opt)
            {
                log_ss << " has INVALID descriptor set " << shader_descriptor_set << " in shader";
                has_invalid_descriptor_sets = true;
            }

            log_ss << ";" << std::endl;
            layout_binding_index++;
        }
    }

    META_LOG("{}", log_ss.str());

    m_vk_descriptor_set_layouts_opt = vk::uniqueToRaw(m_vk_unique_descriptor_set_layouts);

    if (has_invalid_descriptor_sets)
        throw InvalidBindingsException(*this, log_ss.str());
}

} // namespace Methane::Graphics
