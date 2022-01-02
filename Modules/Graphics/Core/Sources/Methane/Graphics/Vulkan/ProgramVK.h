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

#pragma once

#include <Methane/Graphics/ProgramBase.h>

#include <magic_enum.hpp>
#include <vulkan/vulkan.hpp>

#include <array>

namespace Methane::Graphics
{

struct IContextVK;
class ShaderVK;

class ProgramVK final : public ProgramBase
{
public:
    struct DescriptorSetLayoutInfo
    {
        Opt<uint32_t>                               index_opt;
        uint32_t                                    descriptors_count = 0U;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<Argument>                       arguments;              // related arguments for each layout binding
        std::vector<Shader::ByteCodeOffsets>        descriptor_set_offsets; // related descriptor set offsets for each binding/argument
    };

    ProgramVK(const ContextBase& context, const Settings& settings);

    // ObjectBase overrides
    void SetName(const std::string& name) override;

    ShaderVK& GetShaderVK(Shader::Type shader_type) const;
    const IContextVK& GetContextVK() const noexcept;

    std::vector<vk::PipelineShaderStageCreateInfo> GetNativeShaderStageCreateInfos() const;
    vk::PipelineVertexInputStateCreateInfo GetNativeVertexInputStateCreateInfo() const;
    const std::vector<vk::DescriptorSetLayout>& GetNativeDescriptorSetLayouts();
    const vk::DescriptorSetLayout& GetNativeDescriptorSetLayout(Program::ArgumentAccessor::Type argument_access_type);
    const DescriptorSetLayoutInfo& GetDescriptorSetLayoutInfo(Program::ArgumentAccessor::Type argument_access_type);
    const vk::PipelineLayout& GetNativePipelineLayout();
    const vk::DescriptorSet& GetConstantDescriptorSet();
    const vk::DescriptorSet& GetFrameConstantDescriptorSet(Data::Index frame_index);

private:
    using DescriptorSetLayoutInfoByAccessType = std::array<DescriptorSetLayoutInfo, magic_enum::enum_count<Program::ArgumentAccessor::Type>()>;

    void InitializeDescriptorSetLayouts();
    void UpdatePipelineName();

    DescriptorSetLayoutInfoByAccessType        m_descriptor_set_layout_info_by_access_type;
    std::vector<vk::UniqueDescriptorSetLayout> m_vk_unique_descriptor_set_layouts;
    std::vector<vk::DescriptorSetLayout>       m_vk_descriptor_set_layouts;
    vk::UniquePipelineLayout                   m_vk_unique_pipeline_layout;
    std::optional<vk::DescriptorSet>           m_vk_constant_descriptor_set_opt;
    std::vector<vk::DescriptorSet>             m_vk_frame_constant_descriptor_sets;
};

} // namespace Methane::Graphics
