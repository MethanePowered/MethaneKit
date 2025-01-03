/******************************************************************************

Copyright 2019-2024 Evgeny Gorodetskiy

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

struct ArgumentBufferMember
{
    Data::Size        offset;
    Data::Size        array_size;
    Data::Size        buffer_size;
    Rhi::ResourceType resource_type;

    ArgumentBufferMember(MTLStructMember* mtl_struct_member);
};

struct ArgumentBufferLayout
{
    using Member = ArgumentBufferMember;
    using MemberByName = std::map<std::string, Member>;

    Data::Size   data_size = 0U;
    Data::Size   alignment = 0U;
    MemberByName member_by_name;

    ArgumentBufferLayout(id<MTLBufferBinding> mtl_buffer_binding);
    ArgumentBufferLayout() = default;
};

using ArgumentBufferLayouts = std::vector<ArgumentBufferLayout>;

struct IContext;
class Program;

class Shader final
    : public Base::Shader
{
public:
    Shader(Rhi::ShaderType shader_type, const Base::Context& context, const Settings& settings);

    // Base::Shader interface
    Ptrs<Base::ProgramArgumentBinding> GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const final;

    void SetNativeBindings(NSArray<id<MTLBinding>>* mtl_bindings);

    id<MTLFunction>              GetNativeFunction() noexcept                  { return m_mtl_function; }
    const ArgumentBufferLayouts& GetArgumentBufferLayouts() const noexcept     { return m_argument_buffer_layouts; }
    const ArgumentBufferLayout*  GetArgumentBufferLayoutPtr(const Rhi::ProgramArgumentAccessType access_type) const;

    using VertexDescriptorAndStartBufferIndex = std::pair<MTLVertexDescriptor*, Data::Index>;
    VertexDescriptorAndStartBufferIndex GetNativeVertexDescriptor(const Program& program) const;

private:
    const IContext& GetMetalContext() const noexcept;

    static id<MTLFunction> GetMetalLibraryFunction(const IContext& context, const Rhi::ShaderSettings& settings);

    id<MTLFunction>          m_mtl_function;
    NSArray<id<MTLBinding>>* m_mtl_bindings = nil;
    ArgumentBufferLayouts    m_argument_buffer_layouts;
};

} // namespace Methane::Graphics::Metal
