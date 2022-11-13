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

FILE: Methane/Graphics/Metal/Shader.hh
Metal implementation of the shader interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Shader.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

struct IContext;
class Program;

class Shader final : public Base::Shader
{
public:
    Shader(Rhi::ShaderType shader_type, const Base::Context& context, const Settings& settings);

    // Base::Shader interface
    ArgumentBindings GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const final;
    
    id<MTLFunction> GetNativeFunction() noexcept                            { return m_mtl_function; }
    MTLVertexDescriptor* GetNativeVertexDescriptor(const Program& program) const;
    void SetNativeArguments(NSArray<MTLArgument*>* mtl_arguments) noexcept  { m_mtl_arguments = mtl_arguments; }

private:
    const IContext& GetMetalContext() const noexcept;

    id<MTLFunction>        m_mtl_function;
    NSArray<MTLArgument*>* m_mtl_arguments = nil;
};

} // namespace Methane::Graphics::Metal
