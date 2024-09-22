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

#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/ProgramBindings.hh>
#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/IContext.h>
#include <Methane/Graphics/Metal/Device.hh>
#include <Methane/Graphics/Metal/DescriptorManager.hh>
#include <Methane/Graphics/Metal/Types.hh>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/RHI/IRenderContext.h>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#if defined(__MAC_15_0) || defined(__IPHONE_18_0)
#define MTL_PIPELINE_OPTION_BINDING_INFO MTLPipelineOptionBindingInfo
#else
#define MTL_PIPELINE_OPTION_BINDING_INFO MTLPipelineOptionArgumentInfo
#endif

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

    InitArgumentRangeSizesAndConstantRanges();
}

Ptr<Rhi::IProgramBindings> Program::CreateBindings(const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<ProgramBindings>(*this, binding_value_by_argument, frame_index);
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

Data::Size Program::GetArgumentsBufferRangeSize(Rhi::ProgramArgumentAccessType access_type) const noexcept
{
    return m_arguments_range_size_by_access_type[static_cast<uint32_t>(access_type)];
}

const Program::ArgumentsRange& Program::GetFrameConstantArgumentBufferRange(Data::Index frame_index) const
{
    META_CHECK_LESS(frame_index, m_frame_constant_argument_buffer_ranges.size());
    return m_frame_constant_argument_buffer_ranges[frame_index];
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
                                                 options: MTL_PIPELINE_OPTION_BINDING_INFO
                                              reflection: &mtl_render_pipeline_reflection
                                                   error: &ns_error];

    META_CHECK_NOT_NULL_DESCR(mtl_render_pipeline_state,
                                  "Failed to create dummy pipeline state for program reflection: {}",
                                  MacOS::ConvertFromNsString([ns_error localizedDescription]));

    if (!mtl_render_pipeline_reflection)
        return;

    SetNativeShaderArguments(Rhi::ShaderType::Vertex, mtl_render_pipeline_reflection.vertexBindings);
    SetNativeShaderArguments(Rhi::ShaderType::Pixel,  mtl_render_pipeline_reflection.fragmentBindings);
    InitArgumentBindings();
}

void Program::ReflectComputePipelineArguments()
{
    META_FUNCTION_TASK();

    // Create dummy pipeline state to get program reflection of vertex and fragment shader arguments
    MTLComputePipelineDescriptor* mtl_reflection_state_desc = [MTLComputePipelineDescriptor new];
    mtl_reflection_state_desc.computeFunction = GetNativeShaderFunction(Rhi::ShaderType::Compute);

    NSError* ns_error = nil;
    const id<MTLDevice>& mtl_device = GetMetalContext().GetMetalDevice().GetNativeDevice();

    MTLComputePipelineReflection* mtl_compute_pipeline_reflection = nil;
    id<MTLComputePipelineState> mtl_compute_pipeline_state =
        [mtl_device newComputePipelineStateWithDescriptor: mtl_reflection_state_desc
                                                  options: MTL_PIPELINE_OPTION_BINDING_INFO
                                               reflection: &mtl_compute_pipeline_reflection
                                                    error: &ns_error];

    META_CHECK_NOT_NULL_DESCR(mtl_compute_pipeline_state,
                                  "Failed to create compute pipeline state for program reflection: {}",
                                  MacOS::ConvertFromNsString([ns_error localizedDescription]));

    if (!mtl_compute_pipeline_reflection)
        return;

    SetNativeShaderArguments(Rhi::ShaderType::Compute, mtl_compute_pipeline_reflection.bindings);
    InitArgumentBindings();
}

void Program::SetNativeShaderArguments(Rhi::ShaderType shader_type, NSArray<id<MTLBinding>>* mtl_arguments) noexcept
{
    META_FUNCTION_TASK();
    if (HasShader(shader_type))
    {
        GetMetalShader(shader_type).SetNativeBindings(mtl_arguments);
    }
}

void Program::InitArgumentRangeSizesAndConstantRanges()
{
    META_FUNCTION_TASK();
    m_shader_argument_buffer_layouts.clear();
    std::fill(m_arguments_range_size_by_access_type.begin(), m_arguments_range_size_by_access_type.end(), 0U);

    ForEachShader([this](const Base::Shader& shader)
    {
        const auto& metal_shader = static_cast<const Shader&>(shader);
        Data::Index arg_buffer_index = 0U;
        for(const ArgumentBufferLayout& arg_buffer_layout : metal_shader.GetArgumentBufferLayouts())
        {
            META_CHECK_LESS(arg_buffer_index, m_arguments_range_size_by_access_type.size());
            if (!arg_buffer_layout.data_size)
            {
                arg_buffer_index++;
                continue;
            }

            m_shader_argument_buffer_layouts.push_back({
                shader.GetType(),
                arg_buffer_layout.data_size,
                static_cast<Rhi::ProgramArgumentAccessType>(arg_buffer_index)
            });

            m_arguments_range_size_by_access_type[arg_buffer_index] += arg_buffer_layout.data_size;
            arg_buffer_index++;
        }
    });

    const Base::Context& context = GetContext();
    auto& descriptor_manager = static_cast<DescriptorManager&>(GetContext().GetDescriptorManager());

    if (const Data::Size const_args_range_size = GetArgumentsBufferRangeSize(Rhi::ProgramArgumentAccessType::Constant))
    {
        auto& const_args_buffer = descriptor_manager.GetArgumentsBuffer(Rhi::ProgramArgumentAccessType::Constant);
        m_constant_argument_buffer_range = const_args_buffer.ReserveRange(const_args_range_size);
    }

    const uint32_t frames_count = context.GetType() == Rhi::IContext::Type::Render
                                ? dynamic_cast<const Rhi::IRenderContext&>(context).GetSettings().frame_buffers_count
                                : 1U;
    m_frame_constant_argument_buffer_ranges.resize(frames_count);
    std::fill(m_frame_constant_argument_buffer_ranges.begin(), m_frame_constant_argument_buffer_ranges.end(), ArgumentsRange{});

    if (const Data::Size frame_const_args_range_size = GetArgumentsBufferRangeSize(Rhi::ProgramArgumentAccessType::FrameConstant))
    {
        auto& frame_const_args_buffer = descriptor_manager.GetArgumentsBuffer(Rhi::ProgramArgumentAccessType::FrameConstant);
        for(size_t frame_index = 0; frame_index < frames_count; ++frame_index)
        {
            m_frame_constant_argument_buffer_ranges[frame_index] = frame_const_args_buffer.ReserveRange(frame_const_args_range_size);
        }
    }
}

void Program::InitArgumentBindings()
{
    META_FUNCTION_TASK();
    Base::Program::InitArgumentBindings();

    // Update argument buffer offsets of the initialized argument bindings
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        static_cast<ProgramArgumentBinding&>(*argument_binding_ptr).UpdateArgumentBufferOffsets(*this);
    }

    // Update argument buffer offsets in frame argument bindings, except of the first one, which is updated in above loop
    for (const auto& [program_argument, frame_argument_binding_ptrs] : GetFrameArgumentBindings())
        for(Data::Index frame_index = 1; frame_index < frame_argument_binding_ptrs.size(); ++frame_index)
        {
            static_cast<ProgramArgumentBinding&>(*frame_argument_binding_ptrs[frame_index]).UpdateArgumentBufferOffsets(*this);
        }
}

} // namespace Methane::Graphics::Metal
