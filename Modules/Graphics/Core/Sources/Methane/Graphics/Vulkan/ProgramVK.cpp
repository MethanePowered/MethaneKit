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

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

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
    std::fill(m_descriptor_set_layout_index_by_access_type.begin(), m_descriptor_set_layout_index_by_access_type.end(), -1);
    InitArgumentBindings(settings.argument_accessors);
}

void ProgramVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

    ProgramBase::SetName(name);

    const std::string pipeline_name = name + " Pipeline Layout";
    SetVulkanObjectName(GetContextVK().GetDeviceVK().GetNativeDevice(), m_vk_unique_pipeline_layout.get(), pipeline_name.c_str());
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

const std::vector<vk::DescriptorSetLayout>& ProgramVK::GetNativeDescriptorSetLayouts() const
{
    META_FUNCTION_TASK();
    if (!m_vk_descriptor_set_layouts.empty())
        return m_vk_descriptor_set_layouts;

    std::array<std::vector<vk::DescriptorSetLayoutBinding>, magic_enum::enum_count<Program::ArgumentAccessor::Type>()> layout_bindings_by_access_type;
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        const auto& vulkan_argument_binding = dynamic_cast<const ProgramBindingsVK::ArgumentBindingVK&>(*argument_binding_ptr);
        const ProgramBindingsVK::ArgumentBindingVK::SettingsVK& vulkan_binding_settings = vulkan_argument_binding.GetSettingsVK();
        const size_t accessor_type_index = magic_enum::enum_index(vulkan_binding_settings.argument.GetAccessorType()).value();

        layout_bindings_by_access_type[accessor_type_index].emplace_back(
            vulkan_binding_settings.binding,
            vulkan_binding_settings.descriptor_type,
            vulkan_binding_settings.resource_count,
            ShaderVK::ConvertTypeToStageFlagBits(program_argument.GetShaderType())
        );
    }

    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();

    m_vk_unique_descriptor_set_layouts.clear();
    for(size_t layout_index = 0; layout_index < layout_bindings_by_access_type.size(); layout_index++)
    {
        const std::vector<vk::DescriptorSetLayoutBinding>& layout_bindings = layout_bindings_by_access_type[layout_index];
        if (layout_bindings.empty())
            continue;

        m_vk_unique_descriptor_set_layouts.emplace_back(vk_device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, layout_bindings)));
        m_descriptor_set_layout_index_by_access_type[layout_index] = static_cast<int>(m_vk_unique_descriptor_set_layouts.size() - 1);
    }

    m_vk_descriptor_set_layouts = vk::uniqueToRaw(m_vk_unique_descriptor_set_layouts);
    return m_vk_descriptor_set_layouts;
}

const vk::DescriptorSetLayout* ProgramVK::GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type argument_access_type) const noexcept
{
    META_FUNCTION_TASK();
    const int layout_index = m_descriptor_set_layout_index_by_access_type[magic_enum::enum_index(argument_access_type).value()];
    return layout_index >= 0 ? &m_vk_unique_descriptor_set_layouts[layout_index].get() : nullptr;
}

const vk::PipelineLayout& ProgramVK::GetNativePipelineLayout() const
{
    META_FUNCTION_TASK();
    if (m_vk_unique_pipeline_layout)
        return m_vk_unique_pipeline_layout.get();

    const std::vector<vk::DescriptorSetLayout>& vk_descriptor_set_layouts = GetNativeDescriptorSetLayouts();
    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();

    m_vk_unique_pipeline_layout = vk_device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, vk_descriptor_set_layouts));
    return m_vk_unique_pipeline_layout.get();
}

} // namespace Methane::Graphics
