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

#include "ResourceVK.h"

#include <Methane/Graphics/ProgramBindingsBase.h>

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Methane::Graphics
{

struct ICommandListVK;

class ProgramBindingsVK final : public ProgramBindingsBase
{
public:
    class ArgumentBindingVK final : public ArgumentBindingBase
    {
    public:
        struct ByteCodeMap
        {
            Shader::Type shader_type;
            uint32_t     descriptor_set_offset;
            uint32_t     binding_offset;
        };

        using ByteCodeMaps = std::vector<ByteCodeMap>;

        struct SettingsVK : Settings
        {
            vk::DescriptorType descriptor_type;
            ByteCodeMaps       byte_code_maps;
        };

        ArgumentBindingVK(const ContextBase& context, const SettingsVK& settings);
        ArgumentBindingVK(const ArgumentBindingVK& other) = default;

        const SettingsVK& GetSettingsVK() const noexcept                        { return m_settings_vk; }
        const IResourceVK::LocationsVK& GetResourceLocationsVK() const noexcept { return m_resource_locations_vk; }

        void SetDescriptorSetBinding(const vk::DescriptorSet& descriptor_set, uint32_t layout_binding_index) noexcept;
        void SetDescriptorSet(const vk::DescriptorSet& descriptor_set) noexcept;

        // ArgumentBindingBase interface
        void MergeSettings(const ArgumentBindingBase& other) override;

        // ArgumentBinding interface
        const Settings& GetSettings() const noexcept override { return m_settings_vk; }
        void SetResourceLocations(const Resource::Locations& resource_locations) override;

    private:
        SettingsVK               m_settings_vk;
        IResourceVK::LocationsVK m_resource_locations_vk;
        const vk::DescriptorSet* m_vk_descriptor_set_ptr = nullptr;
        uint32_t                 m_vk_binding_value      = 0U;
    };

    ProgramBindingsVK(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index);
    ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument, const Opt<Data::Index>& frame_index);

    // ProgramBindings interface
    void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const override;

    // ProgramBindingsBase interface
    void CompleteInitialization() override { /* not implemented yet */ }

    void Apply(ICommandListVK& command_list, const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior apply_behavior) const;

private:
    std::vector<vk::DescriptorSet> m_descriptor_sets; // descriptor sets corresponding to pipeline layout in the order of their access type
    bool                           m_has_mutable_descriptor_set = false; // if true, then m_descriptor_sets.back() is mutable descriptor set
};

} // namespace Methane::Graphics
