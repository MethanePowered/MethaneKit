/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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
#include "BufferMT.hh"
#include "TextureMT.hh"
#include "SamplerMT.hh"
#include "ContextMT.hh"
#include "DeviceMT.hh"
#include "RenderCommandListMT.hh"
#include "TypesMT.hh"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

#include <cassert>

namespace Methane::Graphics
{

template<typename TMetalResource>
static void SetMetalResource(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const TMetalResource& mtl_buffer, uint32_t arg_index, Data::Size buffer_offset) noexcept;

template<typename TMetalResource>
static void SetMetalResources(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const std::vector<TMetalResource>& mtl_buffer, uint32_t arg_index, const std::vector<NSUInteger>& buffer_offsets) noexcept;

template<>
void SetMetalResource(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const id<MTLBuffer>& mtl_buffer, uint32_t arg_index, Data::Size buffer_offset) noexcept
{
    ITT_FUNCTION_TASK();
    switch(shader_type)
    {
        case Shader::Type::Vertex: [mtl_cmd_encoder setVertexBuffer:mtl_buffer   offset:buffer_offset atIndex:arg_index]; break;
        case Shader::Type::Pixel:  [mtl_cmd_encoder setFragmentBuffer:mtl_buffer offset:buffer_offset atIndex:arg_index]; break;
        default:                    assert(0);
    }
}

template<>
void SetMetalResources(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const std::vector<id<MTLBuffer>>& mtl_buffers,
                      uint32_t arg_index, const std::vector<NSUInteger>& buffer_offsets) noexcept
{
    ITT_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_buffers.size());
    switch(shader_type)
    {
    case Shader::Type::Vertex: [mtl_cmd_encoder setVertexBuffers:mtl_buffers.data()   offsets:buffer_offsets.data() withRange:args_range]; break;
    case Shader::Type::Pixel:  [mtl_cmd_encoder setFragmentBuffers:mtl_buffers.data() offsets:buffer_offsets.data() withRange:args_range]; break;
    default:                    assert(0);
    }
}

template<>
void SetMetalResource(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const id<MTLTexture>& mtl_texture, uint32_t arg_index, Data::Size) noexcept
{
    ITT_FUNCTION_TASK();
    switch(shader_type)
    {
        case Shader::Type::Vertex: [mtl_cmd_encoder setVertexTexture:mtl_texture   atIndex:arg_index]; break;
        case Shader::Type::Pixel:  [mtl_cmd_encoder setFragmentTexture:mtl_texture atIndex:arg_index]; break;
        default:                    assert(0);
    }
}

template<>
void SetMetalResources(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const std::vector<id<MTLTexture>>& mtl_textures,
                      uint32_t arg_index, const std::vector<NSUInteger>&) noexcept
{
    ITT_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_textures.size());
    switch(shader_type)
    {
    case Shader::Type::Vertex: [mtl_cmd_encoder setVertexTextures:mtl_textures.data()   withRange:args_range]; break;
    case Shader::Type::Pixel:  [mtl_cmd_encoder setFragmentTextures:mtl_textures.data() withRange:args_range]; break;
    default:                    assert(0);
    }
}

template<>
void SetMetalResource(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const id<MTLSamplerState>& mtl_sampler, uint32_t arg_index, Data::Size) noexcept
{
    ITT_FUNCTION_TASK();
    switch(shader_type)
    {
        case Shader::Type::Vertex: [mtl_cmd_encoder setVertexSamplerState:mtl_sampler   atIndex:arg_index]; break;
        case Shader::Type::Pixel:  [mtl_cmd_encoder setFragmentSamplerState:mtl_sampler atIndex:arg_index]; break;
        default: assert(0);
    }
}

template<>
void SetMetalResources(Shader::Type shader_type, id<MTLRenderCommandEncoder>& mtl_cmd_encoder, const std::vector<id<MTLSamplerState>>& mtl_samplers,
                      uint32_t arg_index, const std::vector<NSUInteger>&) noexcept
{
    ITT_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_samplers.size());
    switch(shader_type)
    {
    case Shader::Type::Vertex: [mtl_cmd_encoder setVertexSamplerStates:mtl_samplers.data()   withRange:args_range]; break;
    case Shader::Type::Pixel:  [mtl_cmd_encoder setFragmentSamplerStates:mtl_samplers.data() withRange:args_range]; break;
    default:                    assert(0);
    }
}

template<typename TMetalResource>
static void SetMetalResourcesForAll(Shader::Type shader_type, Program& program, id<MTLRenderCommandEncoder>& mtl_cmd_encoder,
                                   const std::vector<TMetalResource>& mtl_resources, uint32_t arg_index,
                                   const std::vector<NSUInteger>& offsets = std::vector<NSUInteger>()) noexcept
{
    ITT_FUNCTION_TASK();
    assert(!mtl_resources.empty());

    if (shader_type == Shader::Type::All)
    {
        if (mtl_resources.size() > 1)
        {
            for (Shader::Type specific_shader_type : program.GetShaderTypes())
            {
                SetMetalResources(specific_shader_type, mtl_cmd_encoder, mtl_resources, arg_index, offsets);
            }
        }
        else
        {
            for (Shader::Type specific_shader_type : program.GetShaderTypes())
            {
                SetMetalResource(specific_shader_type, mtl_cmd_encoder, mtl_resources.back(), arg_index,
                                 offsets.empty() ? 0 : offsets.back());
            }
        }
    }
    else
    {
        if (mtl_resources.size() > 1)
        {
            SetMetalResources(shader_type, mtl_cmd_encoder, mtl_resources, arg_index, offsets);
        }
        else
        {
            SetMetalResource(shader_type, mtl_cmd_encoder, mtl_resources.back(), arg_index,
                             offsets.empty() ? 0 : offsets.back());
        }
    }
}

template<typename NativeResourceT, typename MTLResourceT>
static void GetBoundMetalResources(const Resource::Locations& resource_locations, std::vector<MTLResourceT>& out_mtl_resources, std::vector<uint32_t>& out_offsets)
{
}

Ptr<Program::ResourceBindings> Program::ResourceBindings::Create(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramMT::ResourceBindingsMT>(sp_program, resource_locations_by_argument);
}

Ptr<Program::ResourceBindings> Program::ResourceBindings::CreateCopy(const ResourceBindings& other_resource_bingings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramMT::ResourceBindingsMT>(static_cast<const ProgramMT::ResourceBindingsMT&>(other_resource_bingings), replace_resource_locations_by_argument);
}

ProgramMT::ResourceBindingsMT::ResourceBindingsMT(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
    : ResourceBindingsBase(sp_program, resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

ProgramMT::ResourceBindingsMT::ResourceBindingsMT(const ResourceBindingsMT& other_resource_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : ResourceBindingsBase(other_resource_bindings, replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

void ProgramMT::ResourceBindingsMT::Apply(CommandList& command_list, ApplyBehavior::Mask apply_behavior) const
{
    ITT_FUNCTION_TASK();

    RenderCommandListMT& metal_command_list = dynamic_cast<RenderCommandListMT&>(command_list);
    const CommandListBase::CommandState& command_state = metal_command_list.GetCommandState();
    id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeRenderEncoder();
    
    for(const auto& resource_binding_by_argument : m_resource_binding_by_argument)
    {
        const Argument& program_argument = resource_binding_by_argument.first;
        const ShaderMT::ResourceBindingMT& metal_resource_binding = static_cast<const ShaderMT::ResourceBindingMT&>(*resource_binding_by_argument.second);

        if ((apply_behavior & ApplyBehavior::ConstantOnce || apply_behavior & ApplyBehavior::ChangesOnly) &&
            metal_resource_binding.IsAlreadyApplied(*m_sp_program, program_argument, command_state, apply_behavior & ApplyBehavior::ChangesOnly))
            continue;

        const uint32_t arg_index = metal_resource_binding.GetSettings().argument_index;

        switch(metal_resource_binding.GetSettings().base.resource_type)
        {
            case Resource::Type::Buffer:
                SetMetalResourcesForAll(program_argument.shader_type, *m_sp_program, mtl_cmd_encoder, metal_resource_binding.GetNativeBuffers(), arg_index,
                                       metal_resource_binding.GetBufferOffsets());
                break;

            case Resource::Type::Texture:
                SetMetalResourcesForAll(program_argument.shader_type, *m_sp_program, mtl_cmd_encoder, metal_resource_binding.GetNativeTextures(), arg_index);
                break;

            case Resource::Type::Sampler:
                SetMetalResourcesForAll(program_argument.shader_type, *m_sp_program, mtl_cmd_encoder, metal_resource_binding.GetNativeSamplerStates(), arg_index);
                break;

            default: assert(0);
        }
    }
}

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
        InitResourceBindings(settings.constant_argument_names, settings.addressable_argument_names);
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
