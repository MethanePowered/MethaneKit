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

#include <Methane/Graphics/ProgramBindingsBase.h>

#include <vulkan/vulkan.hpp>

namespace Methane::Graphics
{

struct ICommandListVK;

class ProgramBindingsVK final : public ProgramBindingsBase
{
public:
    class ArgumentBindingVK final : public ArgumentBindingBase
    {
    public:
        struct SettingsVK : Settings
        {
            vk::DescriptorType descriptor_type;
            uint32_t           binding;
        };

        ArgumentBindingVK(const ContextBase& context, const SettingsVK& settings);
        ArgumentBindingVK(const ArgumentBindingVK& other) = default;

        // ArgumentBinding interface
        void SetResourceLocations(const Resource::Locations& resource_locations) override;

        const SettingsVK& GetSettingsVK() const noexcept { return m_settings_vk; }

    private:
        const SettingsVK m_settings_vk;
    };

    ProgramBindingsVK(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument, Data::Index frame_index);
    ProgramBindingsVK(const ProgramBindingsVK& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument, const Opt<Data::Index>& frame_index);

    // ProgramBindings interface
    void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const override;

    // ProgramBindingsBase interface
    void CompleteInitialization() override { /* not implemented yet */ }

    void Apply(ICommandListVK& command_list, const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior apply_behavior) const;

private:
    using DescriptorSetByAccessType = std::array<vk::DescriptorSet, magic_enum::enum_count<Program::ArgumentAccessor::Type>()>;

    DescriptorSetByAccessType m_descriptor_set_by_access_type;
};

} // namespace Methane::Graphics
