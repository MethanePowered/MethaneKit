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

#include <map>

namespace Methane::Graphics::Metal
{

struct ProgramArgumentBindingSettings final
    : Rhi::ProgramArgumentBindingSettings
{
    using StructOffset = uint32_t;
    using StructOffsetByShaderType = std::map<Rhi::ShaderType, StructOffset>;

    uint32_t argument_index;
    StructOffsetByShaderType argument_buffer_offset_by_shader_type;
};

class Program;

class ProgramArgumentBinding final
    : public Base::ProgramArgumentBinding
{
public:
    using Settings             = ProgramArgumentBindingSettings;
    using NativeResources      = std::vector<__unsafe_unretained id<MTLResource>>;
    using NativeBuffers        = std::vector<__unsafe_unretained id<MTLBuffer>>;
    using NativeTextures       = std::vector<__unsafe_unretained id<MTLTexture>>;
    using NativeSamplerStates  = std::vector<__unsafe_unretained id<MTLSamplerState>>;
    using NativeOffsets        = std::vector<NSUInteger>;

    ProgramArgumentBinding(const Base::Context& context, const Settings& settings);

    // Base::ProgramArgumentBinding interface
    [[nodiscard]] Ptr<Base::ProgramArgumentBinding> CreateCopy() const override;
    void MergeSettings(const Base::ProgramArgumentBinding& other) override;

    // IArgumentBinding interface
    bool SetResourceViews(const Rhi::IResource::Views& resource_views) override;

    void UpdateArgumentBufferOffsets(const Program& program);

    bool                       IsArgumentBufferMode() const noexcept   { return !m_settings_mt.argument_buffer_offset_by_shader_type.empty(); }
    const Settings&            GetMetalSettings() const noexcept       { return m_settings_mt; }
    MTLResourceUsage           GetNativeResouceUsage() const noexcept  { return m_mtl_resource_usage; }
    MTLRenderStages            GetNativeRenderStages() const noexcept  { return m_mtl_render_stages; }
    const NativeResources&     GetNativeResources() const noexcept     { return m_mtl_resources; }
    const NativeSamplerStates& GetNativeSamplerStates() const noexcept { return m_mtl_sampler_states; }
    const NativeTextures&      GetNativeTextures() const noexcept      { return m_mtl_textures; }
    const NativeBuffers&       GetNativeBuffers() const noexcept       { return m_mtl_buffers; }
    const NativeOffsets&       GetBufferOffsets() const noexcept       { return m_mtl_buffer_offsets; }

private:
    Settings            m_settings_mt;
    MTLResourceUsage    m_mtl_resource_usage = MTLResourceUsageRead;
    MTLRenderStages     m_mtl_render_stages;
    NativeResources     m_mtl_resources;
    NativeSamplerStates m_mtl_sampler_states;
    NativeTextures      m_mtl_textures;
    NativeBuffers       m_mtl_buffers;
    NativeOffsets       m_mtl_buffer_offsets;
};

} // namespace Methane::Graphics::Metal
