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

FILE: Methane/Graphics/Metal/ProgramBindingsMT.mm
Metal implementation of the program bindings interface.

******************************************************************************/

#include "ProgramBindingsMT.hh"
#include "BufferMT.hh"
#include "TextureMT.hh"
#include "SamplerMT.hh"
#include "RenderCommandListMT.hh"

#include <Methane/Instrumentation.h>

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
static void SetMetalResourcesForAll(Shader::Type shader_type, const Program& program, id<MTLRenderCommandEncoder>& mtl_cmd_encoder,
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

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsMT>(sp_program, resource_locations_by_argument);
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsMT>(static_cast<const ProgramBindingsMT&>(other_program_bindings), replace_resource_locations_by_argument);
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsMT::ArgumentBindingMT>(static_cast<const ProgramBindingsMT::ArgumentBindingMT&>(other_argument_binding));
}

ProgramBindingsMT::ArgumentBindingMT::ArgumentBindingMT(const ContextBase& context, SettingsMT settings)
    : ArgumentBindingBase(context, settings)
    , m_settings_mt(std::move(settings))
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsMT::ArgumentBindingMT::SetResourceLocations(const Resource::Locations& resource_locations)
{
    ITT_FUNCTION_TASK();
    ArgumentBindingBase::SetResourceLocations(resource_locations);

    m_mtl_sampler_states.clear();
    m_mtl_textures.clear();
    m_mtl_buffers.clear();
    m_mtl_buffer_offsets.clear();

    switch(m_settings_mt.resource_type)
    {
    case Resource::Type::Sampler:
        m_mtl_sampler_states.reserve(resource_locations.size());
        std::transform(resource_locations.begin(), resource_locations.end(), std::back_inserter(m_mtl_sampler_states),
                       [](const Resource::Location& resource_location)
                           { return dynamic_cast<const SamplerMT&>(resource_location.GetResource()).GetNativeSamplerState(); });
        break;

    case Resource::Type::Texture:
        m_mtl_textures.reserve(resource_locations.size());
        std::transform(resource_locations.begin(), resource_locations.end(), std::back_inserter(m_mtl_textures),
                       [](const Resource::Location& resource_location)
                           { return dynamic_cast<const TextureMT&>(resource_location.GetResource()).GetNativeTexture(); });
        break;

    case Resource::Type::Buffer:
        m_mtl_buffers.reserve(resource_locations.size());
        m_mtl_buffer_offsets.reserve(resource_locations.size());
        for (const Resource::Location& resource_location : resource_locations)
        {
            m_mtl_buffers.push_back(dynamic_cast<const BufferMT&>(resource_location.GetResource()).GetNativeBuffer());
            m_mtl_buffer_offsets.push_back(static_cast<NSUInteger>(resource_location.GetOffset()));
        }
        break;
    }
}

ProgramBindingsMT::ProgramBindingsMT(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
    : ProgramBindingsBase(sp_program, resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

ProgramBindingsMT::ProgramBindingsMT(const ProgramBindingsMT& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : ProgramBindingsBase(other_program_bindings, replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsMT::Apply(CommandListBase& command_list, ApplyBehavior::Mask apply_behavior) const
{
    ITT_FUNCTION_TASK();

    RenderCommandListMT& metal_command_list = static_cast<RenderCommandListMT&>(command_list);
    id<MTLRenderCommandEncoder>& mtl_cmd_encoder = metal_command_list.GetNativeRenderEncoder();
    
    for(const auto& binding_by_argument : GetArgumentBindings())
    {
        const Program::Argument& program_argument = binding_by_argument.first;
        const ArgumentBindingMT& metal_argument_binding = static_cast<const ArgumentBindingMT&>(*binding_by_argument.second);

        if ((apply_behavior & ApplyBehavior::ConstantOnce || apply_behavior & ApplyBehavior::ChangesOnly) && metal_command_list.GetProgramBindings() &&
            metal_argument_binding.IsAlreadyApplied(GetProgram(), *metal_command_list.GetProgramBindings(), apply_behavior & ApplyBehavior::ChangesOnly))
            continue;

        const uint32_t arg_index = metal_argument_binding.GetSettingsMT().argument_index;

        switch(metal_argument_binding.GetSettingsMT().resource_type)
        {
            case Resource::Type::Buffer:
                SetMetalResourcesForAll(program_argument.shader_type, GetProgram(), mtl_cmd_encoder, metal_argument_binding.GetNativeBuffers(), arg_index,
                                       metal_argument_binding.GetBufferOffsets());
                break;

            case Resource::Type::Texture:
                SetMetalResourcesForAll(program_argument.shader_type, GetProgram(), mtl_cmd_encoder, metal_argument_binding.GetNativeTextures(), arg_index);
                break;

            case Resource::Type::Sampler:
                SetMetalResourcesForAll(program_argument.shader_type, GetProgram(), mtl_cmd_encoder, metal_argument_binding.GetNativeSamplerStates(), arg_index);
                break;
        }
    }
}

} // namespace Methane::Graphics
