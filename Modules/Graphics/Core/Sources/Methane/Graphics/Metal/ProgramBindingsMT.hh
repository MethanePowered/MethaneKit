/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

        using NativeBuffers       = std::vector<__unsafe_unretained id<MTLBuffer>>;
        using NativeTextures      = std::vector<__unsafe_unretained id<MTLTexture>>;
        using NativeSamplerStates = std::vector<__unsafe_unretained id<MTLSamplerState>>;
        using NativeOffsets       = std::vector<NSUInteger>;

        ArgumentBindingMT(const ContextBase& context, const SettingsMT& settings);

        // ArgumentBinding interface
        bool SetResourceViews(const Resource::Views& resource_views) override;

        const SettingsMT&          GetSettingsMT() const noexcept { return m_settings_mt; }
        const NativeSamplerStates& GetNativeSamplerStates() const { return m_mtl_sampler_states; }
        const NativeTextures&      GetNativeTextures() const      { return m_mtl_textures; }
        const NativeBuffers&       GetNativeBuffers() const       { return m_mtl_buffers; }
        const NativeOffsets&       GetBufferOffsets() const       { return m_mtl_buffer_offsets; }

    private:
        const SettingsMT    m_settings_mt;
        NativeSamplerStates m_mtl_sampler_states;
        NativeTextures      m_mtl_textures;
        NativeBuffers       m_mtl_buffers;
        NativeOffsets       m_mtl_buffer_offsets;
    };
    
    ProgramBindingsMT(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index);
    ProgramBindingsMT(const ProgramBindingsMT& other_program_bindings, const ResourceViewsByArgument& replace_resource_view_by_argument, const Opt<Data::Index>& frame_index);

    // ProgramBindings interface
    void Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const override;

    // ProgramBindingsBase interface
    void CompleteInitialization() override { }
};

} // namespace Methane::Graphics
