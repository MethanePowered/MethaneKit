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

FILE: Methane/Graphics/Metal/ProgramMT.hh
Metal implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ProgramBase.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

struct IContextMT;
class ShaderMT;

class ProgramMT final : public ProgramBase
{
public:
    ProgramMT(ContextBase& context, const Settings& settings);
    ~ProgramMT() override;

    ShaderMT& GetShaderMT(Shader::Type shader_type) noexcept;
    
    id<MTLFunction> GetNativeShaderFunction(Shader::Type shader_type) noexcept;
    MTLVertexDescriptor* GetNativeVertexDescriptor() noexcept { return m_mtl_vertex_desc; }

private:
    IContextMT& GetContextMT() noexcept;
    void SetNativeShaderArguments(Shader::Type shader_type, NSArray<MTLArgument*>* mtl_arguments) noexcept;
    
    MTLVertexDescriptor*         m_mtl_vertex_desc = nil;
    id<MTLRenderPipelineState>   m_mtl_dummy_pipeline_state_for_reflection;
    MTLRenderPipelineReflection* m_mtl_render_pipeline_reflection = nil;
};

} // namespace Methane::Graphics
