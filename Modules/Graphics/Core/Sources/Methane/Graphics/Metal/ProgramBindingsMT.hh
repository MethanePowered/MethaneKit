/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Metal/ProgramBindingsMT.hh
Metal implementation of the program bindings interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ProgramBindingsBase.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

class ProgramBindingsMT final : public ProgramBindingsBase
{
public:
    class ArgumentBindingMT final : public ArgumentBindingBase
    {
    public:
        struct SettingsMT final : Settings
        {
            uint32_t argument_index;
        };

        ArgumentBindingMT(const ContextBase& context, SettingsMT settings);
        ArgumentBindingMT(const ArgumentBindingMT& other) = default;

        // ArgumentBinding interface
        void SetResourceLocations(const Resource::Locations& resource_locations) override;

        const SettingsMT&                       GetSettingsMT() const noexcept { return m_settings_mt; }
        const std::vector<id<MTLSamplerState>>& GetNativeSamplerStates() const { return m_mtl_sampler_states; }
        const std::vector<id<MTLTexture>>&      GetNativeTextures() const      { return m_mtl_textures; }
        const std::vector<id<MTLBuffer>>&       GetNativeBuffers() const       { return m_mtl_buffers; }
        const std::vector<NSUInteger>&          GetBufferOffsets() const       { return m_mtl_buffer_offsets; }

    private:
        const SettingsMT                 m_settings_mt;
        std::vector<id<MTLSamplerState>> m_mtl_sampler_states;
        std::vector<id<MTLTexture>>      m_mtl_textures;
        std::vector<id<MTLBuffer>>       m_mtl_buffers;
        std::vector<NSUInteger>          m_mtl_buffer_offsets;
    };
    
    ProgramBindingsMT(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument);
    ProgramBindingsMT(const ProgramBindingsMT& other_program_bindings, const ResourceLocationsByArgument& replace_resource_location_by_argument);

    // ProgramBindings interface
    void Apply(CommandListBase& command_list, ApplyBehavior::Mask apply_behavior) const override;

    // ProgramBindingsBase interface
    void CompleteInitialization() override { }
};

} // namespace Methane::Graphics
