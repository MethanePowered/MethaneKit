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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBindingVK.h
Vulkan implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include "ResourceVK.h"

#include <Methane/Graphics/ProgramBindingsBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct ProgramArgumentBindingSettingsVK
    : ProgramArgumentBindingSettings
{
    struct ByteCodeMap
    {
        ShaderType shader_type;
        uint32_t   descriptor_set_offset;
        uint32_t   binding_offset;
    };

    using ByteCodeMaps = std::vector<ByteCodeMap>;

    vk::DescriptorType descriptor_type;
    ByteCodeMaps       byte_code_maps;
};

class ProgramArgumentBindingVK final
    : public ProgramArgumentBindingBase
{
public:
    using SettingsVK = ProgramArgumentBindingSettingsVK;
    using ByteCodeMap = ProgramArgumentBindingSettingsVK::ByteCodeMap;

    ProgramArgumentBindingVK(const ContextBase& context, const SettingsVK& settings);
    ProgramArgumentBindingVK(const ProgramArgumentBindingVK& other) = default;

    const SettingsVK& GetSettingsVK() const noexcept { return m_settings_vk; }

    void SetDescriptorSetBinding(const vk::DescriptorSet& descriptor_set, uint32_t layout_binding_index) noexcept;
    void SetDescriptorSet(const vk::DescriptorSet& descriptor_set) noexcept;

    // ArgumentBindingBase interface
    void MergeSettings(const ProgramArgumentBindingBase& other) override;

    // IArgumentBinding interface
    const Settings& GetSettings() const noexcept override { return m_settings_vk; }
    bool SetResourceViews(const IResource::Views& resource_views) override;

    void UpdateDescriptorSetsOnGpu();

private:
    SettingsVK                            m_settings_vk;
    const vk::DescriptorSet*              m_vk_descriptor_set_ptr = nullptr;
    uint32_t                              m_vk_binding_value      = 0U;
    vk::WriteDescriptorSet                m_vk_write_descriptor_set;
    std::vector<vk::DescriptorImageInfo>  m_vk_descriptor_images;
    std::vector<vk::DescriptorBufferInfo> m_vk_descriptor_buffers;
    std::vector<vk::BufferView>           m_vk_buffer_views;
};

} // namespace Methane::Graphics