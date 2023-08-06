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

#pragma once

#include "Shader.h"
#include "ProgramBindings.h"

#include <Methane/Graphics/Base/Program.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <vulkan/vulkan.hpp>

#include <array>
#include <mutex>

namespace Methane::Graphics::Vulkan
{

struct IContext;

class Program final
    : public Base::Program
{
public:
    using ByteCodeMap = ProgramArgumentBindingSettings::ByteCodeMap;
    using ByteCodeMaps = ProgramArgumentBindingSettings::ByteCodeMaps;

    struct DescriptorSetLayoutInfo
    {
        Opt<uint32_t>                               index_opt;
        uint32_t                                    descriptors_count = 0U;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<Argument>                       arguments;                    // related arguments for each layout binding
        std::vector<ByteCodeMaps>                   byte_code_maps_for_arguments; // related bytecode maps for each binding/argument
    };

    Program(const Base::Context& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index) override;

    // Base::Object overrides
    bool SetName(std::string_view name) override;

    Shader& GetVulkanShader(Rhi::ShaderType shader_type) const;
    const IContext& GetVulkanContext() const noexcept { return m_vk_context; }

    std::vector<vk::PipelineShaderStageCreateInfo> GetNativeShaderStageCreateInfos() const;
    vk::PipelineVertexInputStateCreateInfo GetNativeVertexInputStateCreateInfo() const;
    const std::vector<vk::DescriptorSetLayout>& GetNativeDescriptorSetLayouts() const;
    const vk::DescriptorSetLayout& GetNativeDescriptorSetLayout(ArgumentAccessor::Type argument_access_type) const;
    const DescriptorSetLayoutInfo& GetDescriptorSetLayoutInfo(ArgumentAccessor::Type argument_access_type) const;
    const vk::PipelineLayout& GetNativePipelineLayout();
    const vk::DescriptorSet& GetConstantDescriptorSet();
    const vk::DescriptorSet& GetFrameConstantDescriptorSet(Data::Index frame_index);

private:
    using DescriptorSetLayoutInfoByAccessType = std::array<DescriptorSetLayoutInfo, magic_enum::enum_count<ArgumentAccessor::Type>()>;

    void InitializeDescriptorSetLayouts();
    void UpdatePipelineName();
    void UpdateDescriptorSetLayoutNames() const;
    void UpdateConstantDescriptorSetName();
    void UpdateFrameConstantDescriptorSetNames() const;

    const IContext&                            m_vk_context;
    DescriptorSetLayoutInfoByAccessType        m_descriptor_set_layout_info_by_access_type;
    std::vector<vk::UniqueDescriptorSetLayout> m_vk_unique_descriptor_set_layouts;
    std::vector<vk::DescriptorSetLayout>       m_vk_descriptor_set_layouts;
    vk::UniquePipelineLayout                   m_vk_unique_pipeline_layout;
    std::optional<vk::DescriptorSet>           m_vk_constant_descriptor_set_opt;
    std::vector<vk::DescriptorSet>             m_vk_frame_constant_descriptor_sets;
    TracyLockable(std::mutex,                  m_mutex);
};

} // namespace Methane::Graphics::Vulkan
