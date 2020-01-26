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

#include "ProgramMT.hh"
#include "ShaderMT.hh"
#include "ContextMT.hh"
#include "DeviceMT.hh"
#include "TypesMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<Program> Program::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramMT>(static_cast<ContextBase&>(context), settings);
}

ProgramMT::ProgramMT(ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
    , m_mtl_vertex_desc(GetShaderMT(Shader::Type::Vertex).GetNativeVertexDescriptor(*this))
{
    ITT_FUNCTION_TASK();

    // In case if RT pixel formats are not set, we assume it renders to frame buffer
    // NOTE: even when program has no pixel shaders render, render state must have at least one color format to be valid
    std::vector<PixelFormat> color_formats = settings.color_formats;
    if (color_formats.empty())
    {
        color_formats.push_back(context.GetSettings().color_format);
    }

    // Create dummy pipeline state to get program reflection of vertex and fragment shader arguments
    MTLRenderPipelineDescriptor* mtl_reflection_state_desc = [MTLRenderPipelineDescriptor new];
    mtl_reflection_state_desc.vertexDescriptor = m_mtl_vertex_desc;
    mtl_reflection_state_desc.vertexFunction   = GetNativeShaderFunction(Shader::Type::Vertex);
    mtl_reflection_state_desc.fragmentFunction = GetNativeShaderFunction(Shader::Type::Pixel);

    // Fill state color attachment descriptors matching program's pixel shader output
    uint32_t attachment_index = 0;
    for(PixelFormat color_format : color_formats)
    {
        mtl_reflection_state_desc.colorAttachments[attachment_index++].pixelFormat = TypeConverterMT::DataFormatToMetalPixelType(color_format);
    }
    mtl_reflection_state_desc.colorAttachments[attachment_index].pixelFormat = MTLPixelFormatInvalid;
    mtl_reflection_state_desc.depthAttachmentPixelFormat = TypeConverterMT::DataFormatToMetalPixelType(m_settings.depth_format);
    
    ContextMT& metal_context = static_cast<ContextMT&>(context);
    
    NSError* ns_error = nil;
    m_mtl_dummy_pipeline_state_for_reflection = [metal_context.GetDeviceMT().GetNativeDevice() newRenderPipelineStateWithDescriptor:mtl_reflection_state_desc options:MTLPipelineOptionArgumentInfo reflection:&m_mtl_render_pipeline_reflection error:&ns_error];
    
    if (ns_error != nil)
    {
        const std::string error_msg = MacOS::ConvertFromNSType<NSString, std::string>([ns_error localizedDescription]);
        throw std::runtime_error("Failed to create dummy pipeline state for program reflection: " + error_msg);
    }

    if (m_mtl_render_pipeline_reflection)
    {
        SetNativeShaderArguments(Shader::Type::Vertex, m_mtl_render_pipeline_reflection.vertexArguments);
        SetNativeShaderArguments(Shader::Type::Pixel,  m_mtl_render_pipeline_reflection.fragmentArguments);
        InitArgumentBindings(settings.argument_descriptions);
    }
}

ProgramMT::~ProgramMT()
{
    ITT_FUNCTION_TASK();

    [m_mtl_vertex_desc release];
}

ContextMT& ProgramMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextMT&>(m_context);
}

ShaderMT& ProgramMT::GetShaderMT(Shader::Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<ShaderMT&>(GetShaderRef(shader_type));
}

id<MTLFunction> ProgramMT::GetNativeShaderFunction(Shader::Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    return HasShader(shader_type) ? static_cast<ShaderMT&>(GetShaderRef(shader_type)).GetNativeFunction() : nil;
}

void ProgramMT::SetNativeShaderArguments(Shader::Type shader_type, NSArray<MTLArgument*>* mtl_arguments) noexcept
{
    ITT_FUNCTION_TASK();
    if (HasShader(shader_type))
    {
        GetShaderMT(shader_type).SetNativeArguments(mtl_arguments);
    }
}

} // namespace Methane::Graphics
