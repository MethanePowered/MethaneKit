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

FILE: Methane/Graphics/Metal/Program.hh
Metal implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Program.h>

#import <Metal/Metal.h>

#include <vector>
#include <array>

namespace Methane::Graphics::Metal
{

struct IContext;
class Shader;

class Program final
    : public Base::Program
{
public:
    using ArgumentsRange = Data::Range<Data::Index>;

    struct ShaderArgumentBufferLayout
    {
        Rhi::ShaderType                shader_type;
        Data::Size                     data_size;
        Rhi::ProgramArgumentAccessType access_type;
    };

    using ShaderArgumentBufferLayouts = std::vector<ShaderArgumentBufferLayout>;

    Program(const Base::Context& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateBindings(const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index) override;

    Shader& GetMetalShader(Rhi::ShaderType shader_type) const;
    
    id<MTLFunction> GetNativeShaderFunction(Rhi::ShaderType shader_type) noexcept;
    MTLVertexDescriptor* GetNativeVertexDescriptor() noexcept { return m_mtl_vertex_desc; }
    Data::Index GetStartVertexBufferIndex() const noexcept { return m_start_vertex_buffer_index; }
    const ShaderArgumentBufferLayouts& GetShaderArgumentBufferLayouts() const noexcept { return m_shader_argument_buffer_layouts; }
    Data::Size GetArgumentsBufferRangeSize(Rhi::ProgramArgumentAccessType access_type) const noexcept;
    const ArgumentsRange& GetConstantArgumentBufferRange() const noexcept { return m_constant_argument_buffer_range; }
    const ArgumentsRange& GetFrameConstantArgumentBufferRange(Data::Index frame_index) const;

private:
    const IContext& GetMetalContext() const noexcept;
    void ReflectRenderPipelineArguments();
    void ReflectComputePipelineArguments();
    void SetNativeShaderArguments(Rhi::ShaderType shader_type, NSArray<id<MTLBinding>>* mtl_arguments) noexcept;
    void InitArgumentRangeSizesAndConstantRanges();

    // Base::Program overrides
    void InitArgumentBindings() override;

    using ArgumentBufferSizeByAccessType = std::array<Data::Size, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()>;

    MTLVertexDescriptor*           m_mtl_vertex_desc = nil;
    Data::Index                    m_start_vertex_buffer_index = 0U;
    ArgumentBufferSizeByAccessType m_arguments_range_size_by_access_type;
    ShaderArgumentBufferLayouts    m_shader_argument_buffer_layouts;
    ArgumentsRange                 m_constant_argument_buffer_range;
    std::vector<ArgumentsRange>    m_frame_constant_argument_buffer_ranges;
};

} // namespace Methane::Graphics::Metal
