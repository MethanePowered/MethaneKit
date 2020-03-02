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

namespace Methane::Graphics
{

struct IContextMT;
class ProgramMT;

class ShaderMT final : public ShaderBase
{
public:
    ShaderMT(Shader::Type shader_type, ContextBase& context, const Settings& settings);
    ~ShaderMT() override;
    
    // ShaderBase interface
    ArgumentBindings GetArgumentBindings(const Program::ArgumentDescriptions& argument_descriptions) const override;
    
    id<MTLFunction>& GetNativeFunction() noexcept                           { return m_mtl_function; }
    MTLVertexDescriptor* GetNativeVertexDescriptor(const ProgramMT& program) const;
    void SetNativeArguments(NSArray<MTLArgument*>* mtl_arguments) noexcept  { m_mtl_arguments = mtl_arguments; }

private:
    IContextMT& GetContextMT() noexcept;

    id<MTLFunction>        m_mtl_function;
    NSArray<MTLArgument*>* m_mtl_arguments = nil;
};

} // namespace Methane::Graphics
