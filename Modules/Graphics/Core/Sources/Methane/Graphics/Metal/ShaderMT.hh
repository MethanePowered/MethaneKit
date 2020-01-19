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

FILE: Methane/Graphics/Metal/ShaderMT.hh
Metal implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ShaderBase.h>

#import <Metal/Metal.h>

#include <string>
#include <memory>

namespace Methane::Graphics
{

class ContextMT;
class ProgramMT;

class ShaderMT : public ShaderBase
{
public:
    class ResourceBindingMT : public ResourceBindingBase
    {
    public:
        struct Settings
        {
            ResourceBindingBase::Settings    base;
            uint32_t                         argument_index;
        };
        
        ResourceBindingMT(ContextBase& context, const Settings& settings);
        ResourceBindingMT(const ResourceBindingMT& other) = default;

        // ResourceBinding interface
        void SetResourceLocations(const Resource::Locations& resource_locations) override;
        
        const Settings& GetSettings() const noexcept { return m_settings; }
        const std::vector<id<MTLSamplerState>>& GetNativeSamplerStates() const { return m_mtl_sampler_states; }
        const std::vector<id<MTLTexture>>&      GetNativeTextures() const      { return m_mtl_textures; }
        const std::vector<id<MTLBuffer>>&       GetNativeBuffers() const       { return m_mtl_buffers; }
        const std::vector<NSUInteger>&          GetBufferOffsets() const       { return m_mtl_buffer_offsets; }
        
    protected:
        const Settings                   m_settings;
        std::vector<id<MTLSamplerState>> m_mtl_sampler_states;
        std::vector<id<MTLTexture>>      m_mtl_textures;
        std::vector<id<MTLBuffer>>       m_mtl_buffers;
        std::vector<NSUInteger>          m_mtl_buffer_offsets;
    };
    
    ShaderMT(Shader::Type shader_type, ContextMT& context, const Settings& settings);
    ~ShaderMT() override;
    
    // ShaderBase interface
    Ptrs<ResourceBinding> GetResourceBindings(const std::set<std::string>& constant_argument_names,
                                              const std::set<std::string>& addressable_argument_names) const override;
    
    id<MTLFunction>& GetNativeFunction() noexcept                           { return m_mtl_function; }
    MTLVertexDescriptor* GetNativeVertexDescriptor(const ProgramMT& program) const;
    void SetNativeArguments(NSArray<MTLArgument*>* mtl_arguments) noexcept  { m_mtl_arguments = mtl_arguments; }

protected:
    ContextMT& GetContextMT() noexcept;

    id<MTLFunction>        m_mtl_function;
    NSArray<MTLArgument*>* m_mtl_arguments = nil;
};

} // namespace Methane::Graphics
