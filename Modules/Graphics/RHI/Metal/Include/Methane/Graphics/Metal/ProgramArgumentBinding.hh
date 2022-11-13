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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBinding.hh
Metal implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/ProgramArgumentBinding.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

struct ProgramArgumentBindingSettings final
    : Rhi::ProgramArgumentBindingSettings
{
    uint32_t argument_index;
};

class ProgramArgumentBinding final
    : public Base::ProgramArgumentBinding
{
public:
    using Settings            = ProgramArgumentBindingSettings;
    using NativeBuffers       = std::vector<__unsafe_unretained id<MTLBuffer>>;
    using NativeTextures      = std::vector<__unsafe_unretained id<MTLTexture>>;
    using NativeSamplerStates = std::vector<__unsafe_unretained id<MTLSamplerState>>;
    using NativeOffsets       = std::vector<NSUInteger>;

    ProgramArgumentBinding(const Base::Context& context, const Settings& settings);

    // IArgumentBinding interface
    bool SetResourceViews(const Rhi::IResource::Views& resource_views) override;

    const Settings&            GetMetalSettings() const noexcept { return m_settings_mt; }
    const NativeSamplerStates& GetNativeSamplerStates() const { return m_mtl_sampler_states; }
    const NativeTextures&      GetNativeTextures() const      { return m_mtl_textures; }
    const NativeBuffers&       GetNativeBuffers() const       { return m_mtl_buffers; }
    const NativeOffsets&       GetBufferOffsets() const       { return m_mtl_buffer_offsets; }

private:
    const Settings      m_settings_mt;
    NativeSamplerStates m_mtl_sampler_states;
    NativeTextures      m_mtl_textures;
    NativeBuffers       m_mtl_buffers;
    NativeOffsets       m_mtl_buffer_offsets;
};

} // namespace Methane::Graphics::Metal
