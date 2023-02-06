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

FILE: Methane/Graphics/Metal/Program.hh
Metal implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Program.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

struct IContext;
class Shader;

class Program final : public Base::Program
{
public:
    Program(const Base::Context& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index) override;

    Shader& GetMetalShader(Rhi::ShaderType shader_type) noexcept;
    
    id<MTLFunction> GetNativeShaderFunction(Rhi::ShaderType shader_type) noexcept;
    MTLVertexDescriptor* GetNativeVertexDescriptor() noexcept { return m_mtl_vertex_desc; }

private:
    const IContext& GetMetalContext() const noexcept;
    void SetNativeShaderArguments(Rhi::ShaderType shader_type, NSArray<MTLArgument*>* mtl_arguments) noexcept;
    
    MTLVertexDescriptor*         m_mtl_vertex_desc = nil;
    id<MTLRenderPipelineState>   m_mtl_dummy_pipeline_state_for_reflection;
};

} // namespace Methane::Graphics::Metal
