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

namespace Methane::Graphics::Metal
{

struct IContext;
class Shader;

class Program final
    : public Base::Program
{
public:
    struct ShaderArgumentBufferLayout
    {
        Rhi::ShaderType shader_type;
        Data::Size      data_size;
        Data::Index     index;
    };

    using ShaderArgumentBufferLayouts = std::vector<ShaderArgumentBufferLayout>;

    Program(const Base::Context& context, const Settings& settings);

    // IProgram interface
    [[nodiscard]] Ptr<Rhi::IProgramBindings> CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index) override;

    Shader& GetMetalShader(Rhi::ShaderType shader_type) const;
    
    id<MTLFunction> GetNativeShaderFunction(Rhi::ShaderType shader_type) noexcept;
    MTLVertexDescriptor* GetNativeVertexDescriptor() noexcept { return m_mtl_vertex_desc; }
    Data::Index GetStartVertexBufferIndex() const noexcept { return m_start_vertex_buffer_index; }
    Data::Size GetArgumentBuffersSize() const noexcept { return m_argument_buffers_size; }
    const ShaderArgumentBufferLayouts& GetShaderArgumentBufferLayouts() const noexcept { return m_shader_argument_buffer_layouts; }

private:
    const IContext& GetMetalContext() const noexcept;
    void ReflectRenderPipelineArguments();
    void ReflectComputePipelineArguments();
    void SetNativeShaderArguments(Rhi::ShaderType shader_type, NSArray<id<MTLBinding>>* mtl_arguments) noexcept;
    void InitArgumentBuffersSize();

    // Base::Program overrides
    void InitArgumentBindings() override;
    Ptr<ArgumentBinding> CreateArgumentBindingInstance(const Ptr<ArgumentBinding>& argument_binding_ptr, Data::Index frame_index) const override;

    MTLVertexDescriptor*        m_mtl_vertex_desc = nil;
    Data::Index                 m_start_vertex_buffer_index = 0U;
    Data::Size                  m_argument_buffers_size = 0U;
    ShaderArgumentBufferLayouts m_shader_argument_buffer_layouts;
};

} // namespace Methane::Graphics::Metal
