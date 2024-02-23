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

#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/ProgramBindings.hh>
#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/IContext.h>
#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/Types.hh>
#include <Methane/Graphics/Base/Context.h>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

Program::Program(const Base::Context& context, const Settings& settings)
    : Base::Program(context, settings)
{
    META_FUNCTION_TASK();
    if (HasShader(Rhi::ShaderType::Vertex))
        ReflectRenderPipelineArguments();
    else if (HasShader(Rhi::ShaderType::Compute))
        ReflectComputePipelineArguments();

    InitArgumentBuffersSize();
}

Ptr<Rhi::IProgramBindings> Program::CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<ProgramBindings>(*this, resource_views_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

const IContext& Program::GetMetalContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const IContext&>(GetContext());
}

Shader& Program::GetMetalShader(Rhi::ShaderType shader_type) const
{
    META_FUNCTION_TASK();
    return static_cast<Shader&>(GetShaderRef(shader_type));
}

id<MTLFunction> Program::GetNativeShaderFunction(Rhi::ShaderType shader_type) noexcept
{
    META_FUNCTION_TASK();
    return HasShader(shader_type) ? static_cast<Shader&>(GetShaderRef(shader_type)).GetNativeFunction() : nil;
}

void Program::ReflectRenderPipelineArguments()
{
    META_FUNCTION_TASK();
    const Settings& settings = Base::Program::GetSettings();

    std::tie(m_mtl_vertex_desc, m_start_vertex_buffer_index) = GetMetalShader(Rhi::ShaderType::Vertex).GetNativeVertexDescriptor(*this);

    // Create dummy pipeline state to get program reflection of vertex and fragment shader arguments
    MTLRenderPipelineDescriptor* mtl_reflection_state_desc = [MTLRenderPipelineDescriptor new];
    mtl_reflection_state_desc.vertexDescriptor = m_mtl_vertex_desc;
    mtl_reflection_state_desc.vertexFunction   = GetNativeShaderFunction(Rhi::ShaderType::Vertex);
    mtl_reflection_state_desc.fragmentFunction = GetNativeShaderFunction(Rhi::ShaderType::Pixel);

    // Fill state color attachment descriptors matching program's pixel shader output
    // NOTE: even when program has no pixel shaders render, render state must have at least one color format to be valid
    uint32_t attachment_index = 0;
    for(PixelFormat color_format : settings.attachment_formats.colors)
    {
        mtl_reflection_state_desc.colorAttachments[attachment_index++].pixelFormat = TypeConverter::DataFormatToMetalPixelType(color_format);
    }
    mtl_reflection_state_desc.colorAttachments[attachment_index].pixelFormat = MTLPixelFormatInvalid;
    mtl_reflection_state_desc.depthAttachmentPixelFormat   = TypeConverter::DataFormatToMetalPixelType(settings.attachment_formats.depth);
    mtl_reflection_state_desc.stencilAttachmentPixelFormat = TypeConverter::DataFormatToMetalPixelType(settings.attachment_formats.stencil);

    NSError* ns_error = nil;
    const id<MTLDevice>& mtl_device = GetMetalContext().GetMetalDevice().GetNativeDevice();

    MTLRenderPipelineReflection* mtl_render_pipeline_reflection = nil;
    id<MTLRenderPipelineState> mtl_render_pipeline_state =
        [mtl_device newRenderPipelineStateWithDescriptor: mtl_reflection_state_desc
                                                 options: MTLPipelineOptionArgumentInfo
                                              reflection: &mtl_render_pipeline_reflection
                                                   error: &ns_error];

    META_CHECK_ARG_NOT_NULL_DESCR(mtl_render_pipeline_state,
                                  "Failed to create dummy pipeline state for program reflection: {}",
                                  MacOS::ConvertFromNsString([ns_error localizedDescription]));

    if (!mtl_render_pipeline_reflection)
        return;

    SetNativeShaderArguments(Rhi::ShaderType::Vertex, mtl_render_pipeline_reflection.vertexBindings);
    SetNativeShaderArguments(Rhi::ShaderType::Pixel,  mtl_render_pipeline_reflection.fragmentBindings);
    InitArgumentBindings(settings.argument_accessors);
}

void Program::ReflectComputePipelineArguments()
{
    META_FUNCTION_TASK();
    const Settings& settings = Base::Program::GetSettings();

    // Create dummy pipeline state to get program reflection of vertex and fragment shader arguments
    MTLComputePipelineDescriptor* mtl_reflection_state_desc = [MTLComputePipelineDescriptor new];
    mtl_reflection_state_desc.computeFunction = GetNativeShaderFunction(Rhi::ShaderType::Compute);

    NSError* ns_error = nil;
    const id<MTLDevice>& mtl_device = GetMetalContext().GetMetalDevice().GetNativeDevice();

    MTLComputePipelineReflection* mtl_compute_pipeline_reflection = nil;
    id<MTLComputePipelineState> mtl_compute_pipeline_state =
        [mtl_device newComputePipelineStateWithDescriptor: mtl_reflection_state_desc
                                                  options: MTLPipelineOptionArgumentInfo
                                               reflection: &mtl_compute_pipeline_reflection
                                                    error: &ns_error];

    META_CHECK_ARG_NOT_NULL_DESCR(mtl_compute_pipeline_state,
                                  "Failed to create compute pipeline state for program reflection: {}",
                                  MacOS::ConvertFromNsString([ns_error localizedDescription]));

    if (!mtl_compute_pipeline_reflection)
        return;

    SetNativeShaderArguments(Rhi::ShaderType::Compute, mtl_compute_pipeline_reflection.bindings);
    InitArgumentBindings(settings.argument_accessors);
}

void Program::SetNativeShaderArguments(Rhi::ShaderType shader_type, NSArray<id<MTLBinding>>* mtl_arguments) noexcept
{
    META_FUNCTION_TASK();
    if (HasShader(shader_type))
    {
        GetMetalShader(shader_type).SetNativeBindings(mtl_arguments);
    }
}

void Program::InitArgumentBuffersSize()
{
    META_FUNCTION_TASK();
    m_argument_buffers_size = 0U;
    ForEachShader([this](const Base::Shader& shader)
    {
        const auto& metal_shader = static_cast<const Shader&>(shader);
        Data::Index layout_index = 0U;
        for(const ArgumentBufferLayout& layout : metal_shader.GetArgumentBufferLayouts())
        {
            m_shader_argument_buffer_layouts.push_back({ shader.GetType(), layout.data_size, layout_index });
            m_argument_buffers_size += layout.data_size;
            layout_index++;
        }
    });
}

void Program::InitArgumentBindings(const ArgumentAccessors& argument_accessors)
{
    META_FUNCTION_TASK();
    Base::Program::InitArgumentBindings(argument_accessors);

    // Update argument buffer offsets of the initialized argument bindings
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        static_cast<ProgramArgumentBinding&>(*argument_binding_ptr).UpdateArgumentBufferOffsets(*this);
    }
}

Ptr<Base::ProgramArgumentBinding> Program::CreateArgumentBindingInstance(const Ptr<Base::ProgramArgumentBinding>& argument_binding_ptr, Data::Index frame_index) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(argument_binding_ptr);

    // Argument Bindings which are using Metal Argument Buffer, always create a copy.
    // TODO: Change this when type of argument binding (Mutable, Constant, FrameConstant) will be defined on shader level
    //       and thus will result in splitting of arguments to separate argument buffers.
    auto& metal_argument_binding = static_cast<ProgramArgumentBinding&>(*argument_binding_ptr);
    if (metal_argument_binding.IsArgumentBufferMode())
    {
        return metal_argument_binding.CreateCopy();
    }

    return Base::Program::CreateArgumentBindingInstance(argument_binding_ptr, frame_index);
}

} // namespace Methane::Graphics::Metal
