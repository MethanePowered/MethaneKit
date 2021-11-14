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

const vk::DescriptorSetLayout& ProgramVK::GetNativeDescriptorSetLayout() const
{
    META_FUNCTION_TASK();
    if (m_vk_unique_descriptor_set_layout)
        return m_vk_unique_descriptor_set_layout.get();

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        const auto& vulkan_argument_binding = dynamic_cast<const ProgramBindingsVK::ArgumentBindingVK&>(*argument_binding_ptr);
        const ProgramBindingsVK::ArgumentBindingVK::SettingsVK& vulkan_binding_settings = vulkan_argument_binding.GetSettingsVK();
        layout_bindings.emplace_back(
            vulkan_binding_settings.binding,
            vulkan_binding_settings.descriptor_type,
            vulkan_binding_settings.resource_count,
            ShaderVK::ConvertTypeToStageFlagBits(program_argument.GetShaderType())
        );
    }

    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();
    m_vk_unique_descriptor_set_layout = vk_device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, layout_bindings));
    return m_vk_unique_descriptor_set_layout.get();
}

const vk::PipelineLayout& ProgramVK::GetNativePipelineLayout() const
{
    META_FUNCTION_TASK();
    if (m_vk_unique_pipeline_layout)
        return m_vk_unique_pipeline_layout.get();

    const vk::Device& vk_device = GetContextVK().GetDeviceVK().GetNativeDevice();
    const vk::DescriptorSetLayout& vk_descriptor_set_layout = GetNativeDescriptorSetLayout();

    m_vk_unique_pipeline_layout = vk_device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, 1, &vk_descriptor_set_layout));
    return m_vk_unique_pipeline_layout.get();
}

} // namespace Methane::Graphics
