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

FILE: Methane/Graphics/Metal/ProgramBindings.mm
Metal implementation of the program bindings interface.

******************************************************************************/

#include <Methane/Graphics/Metal/ProgramBindings.hh>
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/Buffer.hh>
#include <Methane/Graphics/Metal/Texture.hh>
#include <Methane/Graphics/Metal/Sampler.hh>
#include <Methane/Graphics/Metal/DescriptorManager.hh>
#include <Methane/Graphics/Metal/RenderCommandList.hh>
#include <Methane/Graphics/Metal/ComputeCommandList.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

using NativeBuffers = ProgramArgumentBinding::NativeBuffers;
using NativeTextures = ProgramArgumentBinding::NativeTextures;
using NativeSamplerStates = ProgramArgumentBinding::NativeSamplerStates;
using NativeOffsets = ProgramArgumentBinding::NativeOffsets;

constexpr ProgramBindings::ApplyBehaviorMask g_constant_once_and_changes_only({
    ProgramBindings::ApplyBehavior::ConstantOnce,
    ProgramBindings::ApplyBehavior::ChangesOnly
});

template<typename TMetalResource>
void SetRenderResource(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                       TMetalResource mtl_buffer, uint32_t arg_index, NSUInteger buffer_offset);

template<typename TMetalResource>
void SetRenderResources(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                        const std::vector<TMetalResource>& mtl_buffer, uint32_t arg_index,
                        const std::vector<NSUInteger>& buffer_offsets);

template<>
void SetRenderResource(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                       __unsafe_unretained id <MTLBuffer> mtl_buffer, uint32_t arg_index, NSUInteger buffer_offset)
{
    META_FUNCTION_TASK();
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexBuffer:mtl_buffer offset:buffer_offset atIndex:arg_index];
            break;
        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentBuffer:mtl_buffer offset:buffer_offset atIndex:arg_index];
            break;
        default:
            META_UNEXPECTED_ARG(shader_type);
    }
}

template<>
void SetRenderResources(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                        const NativeBuffers& mtl_buffers, uint32_t arg_index,
                        const std::vector<NSUInteger>& buffer_offsets)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_buffers.size());
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexBuffers:mtl_buffers.data() offsets:buffer_offsets.data() withRange:args_range];
            break;
        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentBuffers:mtl_buffers.data() offsets:buffer_offsets.data() withRange:args_range];
            break;
        default:
            META_UNEXPECTED_ARG(shader_type);
    }
}

template<>
void SetRenderResource(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                       __unsafe_unretained id <MTLTexture> mtl_texture, uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexTexture:mtl_texture atIndex:arg_index];
            break;
        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentTexture:mtl_texture atIndex:arg_index];
            break;
        default:
            META_UNEXPECTED_ARG(shader_type);
    }
}

template<>
void SetRenderResources(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                        const NativeTextures& mtl_textures, uint32_t arg_index, const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_textures.size());
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexTextures:mtl_textures.data() withRange:args_range];
            break;
        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentTextures:mtl_textures.data() withRange:args_range];
            break;
        default:
            META_UNEXPECTED_ARG(shader_type);
    }
}

template<>
void SetRenderResource(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                       __unsafe_unretained id <MTLSamplerState> mtl_sampler, uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexSamplerState:mtl_sampler atIndex:arg_index];
            break;
        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentSamplerState:mtl_sampler atIndex:arg_index];
            break;
        default:
            META_UNEXPECTED_ARG(shader_type);
    }
}

template<>
void SetRenderResources(Rhi::ShaderType shader_type, const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                        const NativeSamplerStates& mtl_samplers, uint32_t arg_index, const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_samplers.size());
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexSamplerStates:mtl_samplers.data() withRange:args_range];
            break;
        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentSamplerStates:mtl_samplers.data() withRange:args_range];
            break;
        default:
            META_UNEXPECTED_ARG(shader_type);
    }
}

template<typename TMetalResource>
void SetRenderResourcesForAll(Rhi::ShaderType shader_type, const Rhi::IProgram& program,
                              const id <MTLRenderCommandEncoder>& mtl_cmd_encoder,
                              const std::vector<TMetalResource>& mtl_resources, uint32_t arg_index,
                              const std::vector<NSUInteger>& offsets = std::vector<NSUInteger>())
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY(mtl_resources);

    if (shader_type == Rhi::ShaderType::All)
    {
        if (mtl_resources.size() > 1)
        {
            for (Rhi::ShaderType specific_shader_type: program.GetShaderTypes())
            {
                SetRenderResources(specific_shader_type, mtl_cmd_encoder, mtl_resources, arg_index, offsets);
            }
        }
        else
        {
            for (Rhi::ShaderType specific_shader_type: program.GetShaderTypes())
            {
                SetRenderResource(specific_shader_type, mtl_cmd_encoder, mtl_resources.back(), arg_index,
                                  offsets.empty() ? 0 : offsets.back());
            }
        }
    }
    else
    {
        if (mtl_resources.size() > 1)
        {
            SetRenderResources(shader_type, mtl_cmd_encoder, mtl_resources, arg_index, offsets);
        }
        else
        {
            SetRenderResource(shader_type, mtl_cmd_encoder, mtl_resources.back(), arg_index,
                              offsets.empty() ? 0 : offsets.back());
        }
    }
}

template<typename TMetalResource>
void SetComputeResource(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                        TMetalResource mtl_buffer, uint32_t arg_index, NSUInteger buffer_offset);

template<typename TMetalResource>
void SetComputeResources(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                         const std::vector<TMetalResource>& mtl_buffer, uint32_t arg_index,
                         const std::vector<NSUInteger>& buffer_offsets);

template<>
void SetComputeResource(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                        __unsafe_unretained id <MTLBuffer> mtl_buffer, uint32_t arg_index, NSUInteger buffer_offset)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setBuffer:mtl_buffer offset:buffer_offset atIndex:arg_index];
}

template<>
void SetComputeResources(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                         const NativeBuffers& mtl_buffers, uint32_t arg_index,
                         const std::vector<NSUInteger>& buffer_offsets)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_buffers.size());
    [mtl_cmd_encoder setBuffers:mtl_buffers.data() offsets:buffer_offsets.data() withRange:args_range];
}

template<>
void SetComputeResource(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                        __unsafe_unretained id <MTLTexture> mtl_texture, uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setTexture:mtl_texture atIndex:arg_index];
}

template<>
void SetComputeResources(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                         const NativeTextures& mtl_textures, uint32_t arg_index, const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_textures.size());
    [mtl_cmd_encoder setTextures:mtl_textures.data() withRange:args_range];
}

template<>
void SetComputeResource(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                        __unsafe_unretained id <MTLSamplerState> mtl_sampler, uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setSamplerState:mtl_sampler atIndex:arg_index];
}

template<>
void SetComputeResources(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                         const NativeSamplerStates& mtl_samplers, uint32_t arg_index, const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_samplers.size());
    [mtl_cmd_encoder setSamplerStates:mtl_samplers.data() withRange:args_range];
}

template<typename TMetalResource>
void SetComputeResourcesForAll(const id <MTLComputeCommandEncoder>& mtl_cmd_encoder,
                               const std::vector<TMetalResource>& mtl_resources, uint32_t arg_index,
                               const std::vector<NSUInteger>& offsets = std::vector<NSUInteger>())
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY(mtl_resources);
    if (mtl_resources.size() > 1)
    {
        SetComputeResources(mtl_cmd_encoder, mtl_resources, arg_index, offsets);
    }
    else
    {
        SetComputeResource(mtl_cmd_encoder, mtl_resources.back(), arg_index,
                           offsets.empty() ? 0 : offsets.back());
    }
}

static void WriteNativeBufferAddresses(const NativeBuffers& native_buffers, const NativeOffsets& offsets, uint64_t* buffer_ptr)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(native_buffers.size(), offsets.size());
    for(size_t buffer_index = 0; buffer_index < native_buffers.size(); buffer_index++)
    {
        const auto& native_buffer = native_buffers[buffer_index];
        const uint64_t offset = offsets[buffer_index];
        *buffer_ptr = native_buffer.gpuAddress + offset;
        buffer_ptr++;
    }
}

static void WriteNativeTextureIds(const NativeTextures& native_textures, uint64_t* buffer_ptr)
{
    META_FUNCTION_TASK();
    for(const auto& native_texture : native_textures)
    {
        *buffer_ptr = native_texture.gpuResourceID._impl;
        buffer_ptr++;
    }
}

static void WriteNativeSamplerIds(const NativeSamplerStates& native_samplers, uint64_t* buffer_ptr)
{
    META_FUNCTION_TASK();
    for(const auto& native_sampler : native_samplers)
    {
        *buffer_ptr = native_sampler.gpuResourceID._impl;
        buffer_ptr++;
    }
}

static void WriteArgumentBindingResourceIds(const ProgramArgumentBinding& arg_binding, uint64_t* buffer_ptr)
{
    META_FUNCTION_TASK();
    Rhi::ResourceType resource_type = arg_binding.GetSettings().resource_type;
    switch(resource_type)
    {
        case Rhi::ResourceType::Buffer:
            WriteNativeBufferAddresses(arg_binding.GetNativeBuffers(), arg_binding.GetBufferOffsets(), buffer_ptr);
            break;
        case Rhi::ResourceType::Texture:
            WriteNativeTextureIds(arg_binding.GetNativeTextures(), buffer_ptr);
            break;
        case Rhi::ResourceType::Sampler:
            WriteNativeSamplerIds(arg_binding.GetNativeSamplerStates(), buffer_ptr);
            break;
        default:
            META_UNEXPECTED_ARG(resource_type);
    }
}

ProgramBindings::ProgramBindings(Program& program, const ResourceViewsByArgument& resource_views_by_argument,
                                 Data::Index frame_index)
    : Base::ProgramBindings(program, resource_views_by_argument, frame_index)
{ }

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings,
                                 const ResourceViewsByArgument& replace_resource_views_by_argument,
                                 const Opt<Data::Index>& frame_index)
    : Base::ProgramBindings(other_program_bindings, replace_resource_views_by_argument, frame_index)
{ }

Ptr<Rhi::IProgramBindings> ProgramBindings::CreateCopy(const ResourceViewsByArgument& replace_resource_views_by_argument,
                                                       const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindings>(*this, replace_resource_views_by_argument, frame_index);
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    const Rhi::CommandListType command_list_type = command_list.GetType();
    switch (command_list_type)
    {
        case Rhi::CommandListType::Render:
            Apply(static_cast<RenderCommandList&>(command_list), apply_behavior);
            break;
        case Rhi::CommandListType::Compute:
            Apply(static_cast<ComputeCommandList&>(command_list), apply_behavior);
            break;
        default:
            META_UNEXPECTED_ARG(command_list_type);
    }
}

void ProgramBindings::CompleteInitialization(Data::Bytes& argument_buffer_data, const ArgumentsRange& arg_range)
{
    META_FUNCTION_TASK();
    m_argument_buffer_range = arg_range;
    Data::Byte* arg_data_ptr = argument_buffer_data.data() + arg_range.GetStart();
    for (const auto& [program_arg, arg_binding_ptr]: GetArgumentBindings())
    {
        const auto& metal_argument_binding = static_cast<const ArgumentBinding&>(*arg_binding_ptr);
        for(const auto& [shader_type, arg_offset] : metal_argument_binding.GetMetalSettings().argument_buffer_offset_by_shader_type)
        {
            WriteArgumentBindingResourceIds(metal_argument_binding, reinterpret_cast<uint64_t*>(arg_data_ptr + arg_offset));
        }
    }
}

template<typename FuncType> // function void(const ArgumentBinding&)
void ProgramBindings::ForEachChangedArgumentBinding(const Base::ProgramBindings* applied_program_bindings_ptr,
                                                    ApplyBehaviorMask apply_behavior, FuncType functor) const
{
    for (const auto& [program_arg, arg_binding_ptr]: GetArgumentBindings())
    {
        const auto& metal_argument_binding = static_cast<const ArgumentBinding&>(*arg_binding_ptr);
        if (apply_behavior.HasAnyBits(g_constant_once_and_changes_only) && applied_program_bindings_ptr &&
            metal_argument_binding.IsAlreadyApplied(GetProgram(), *applied_program_bindings_ptr,
                                                    apply_behavior.HasAnyBit(ApplyBehavior::ChangesOnly)))
            continue;

        functor(metal_argument_binding);
    }
}

void ProgramBindings::SetRenderResources(const id<MTLRenderCommandEncoder>& mtl_cmd_encoder,
                                         const Base::ProgramBindings* applied_program_bindings_ptr,
                                         ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    Rhi::IProgram& program = GetProgram();
    ForEachChangedArgumentBinding(applied_program_bindings_ptr, apply_behavior,
        [&mtl_cmd_encoder, &program](const ArgumentBinding& argument_binding)
        {
            const ProgramArgumentBinding::Settings& settings = argument_binding.GetMetalSettings();
            switch (settings.resource_type)
            {
                case Rhi::ResourceType::Buffer:
                    SetRenderResourcesForAll(settings.argument.GetShaderType(),
                                             program,
                                             mtl_cmd_encoder,
                                             argument_binding.GetNativeBuffers(),
                                             settings.argument_index,
                                             argument_binding.GetBufferOffsets());
                    break;

                case Rhi::ResourceType::Texture:
                    SetRenderResourcesForAll(settings.argument.GetShaderType(),
                                             program,
                                             mtl_cmd_encoder,
                                             argument_binding.GetNativeTextures(),
                                             settings.argument_index);
                    break;

                case Rhi::ResourceType::Sampler:
                    SetRenderResourcesForAll(settings.argument.GetShaderType(),
                                             program,
                                             mtl_cmd_encoder,
                                             argument_binding.GetNativeSamplerStates(),
                                             settings.argument_index);
                    break;

                default:
                    META_UNEXPECTED_ARG(settings.resource_type);
            }
        }
    );
}

void ProgramBindings::SetComputeResources(const id<MTLComputeCommandEncoder>& mtl_cmd_encoder,
                                          const Base::ProgramBindings* applied_program_bindings_ptr,
                                          ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    ForEachChangedArgumentBinding(applied_program_bindings_ptr, apply_behavior,
        [&mtl_cmd_encoder](const ArgumentBinding& argument_binding)
        {
            const ProgramArgumentBinding::Settings& settings = argument_binding.GetMetalSettings();
            switch (settings.resource_type)
            {
                case Rhi::ResourceType::Buffer:
                    SetComputeResourcesForAll(mtl_cmd_encoder,
                                              argument_binding.GetNativeBuffers(),
                                              settings.argument_index,
                                              argument_binding.GetBufferOffsets());
                    break;

                case Rhi::ResourceType::Texture:
                    SetComputeResourcesForAll(mtl_cmd_encoder,
                                              argument_binding.GetNativeTextures(),
                                              settings.argument_index);
                    break;

                case Rhi::ResourceType::Sampler:
                    SetComputeResourcesForAll(mtl_cmd_encoder,
                                              argument_binding.GetNativeSamplerStates(),
                                              settings.argument_index);
                    break;

                default:
                    META_UNEXPECTED_ARG(settings.resource_type);
            }
        }
    );
}

void ProgramBindings::SetRenderArgumentBuffers(const id<MTLRenderCommandEncoder>& mtl_cmd_encoder) const
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<Program&>(GetProgram());
    auto& descriptor_manager = static_cast<DescriptorManager&>(program.GetContext().GetDescriptorManager());
    const auto* argument_buffer_ptr = static_cast<const Buffer*>(descriptor_manager.GetArgumentBuffer());
    META_CHECK_ARG_NOT_NULL_DESCR(argument_buffer_ptr, "Argument Buffer is not initialized in Descriptor Manager.");

    const id<MTLBuffer>& mtl_argument_buffer = argument_buffer_ptr->GetNativeBuffer();
    for(const Program::ShaderArgumentBufferLayout& arg_layout : program.GetShaderArgumentBufferLayouts())
    {
        const Data::Index arg_buffer_offset = m_argument_buffer_range.GetStart() + arg_layout.data_size;
        META_CHECK_ARG_LESS_OR_EQUAL_DESCR(arg_buffer_offset, m_argument_buffer_range.GetEnd(), "invalid offset in argument buffer");
        switch(arg_layout.shader_type)
        {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexBuffer:mtl_argument_buffer
                                      offset:arg_buffer_offset
                                     atIndex:arg_layout.index];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentBuffer:mtl_argument_buffer
                                        offset:arg_buffer_offset
                                       atIndex:arg_layout.index];
            break;

        default:
            META_UNEXPECTED_ARG(arg_layout.shader_type);
        }
    }
}

void ProgramBindings::SetComputeArgumentBuffers(const id<MTLComputeCommandEncoder>& mtl_cmd_encoder) const
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<Program&>(GetProgram());
    auto& descriptor_manager = static_cast<DescriptorManager&>(program.GetContext().GetDescriptorManager());
    const auto* argument_buffer_ptr = static_cast<const Buffer*>(descriptor_manager.GetArgumentBuffer());
    META_CHECK_ARG_NOT_NULL_DESCR(argument_buffer_ptr, "Argument Buffer is not initialized in Descriptor Manager.");

    const id<MTLBuffer>& mtl_argument_buffer = argument_buffer_ptr->GetNativeBuffer();
    for(const Program::ShaderArgumentBufferLayout& arg_layout : program.GetShaderArgumentBufferLayouts())
    {
        const Data::Index arg_buffer_offset = m_argument_buffer_range.GetStart() + arg_layout.data_size;
        META_CHECK_ARG_LESS_DESCR(arg_buffer_offset, m_argument_buffer_range.GetEnd(), "invalid offset in argument buffer");
        META_CHECK_ARG_EQUAL(arg_layout.shader_type, Rhi::ShaderType::Compute);
        [mtl_cmd_encoder setBuffer:mtl_argument_buffer
                            offset:arg_buffer_offset
                           atIndex:arg_layout.index];
    }
}

void ProgramBindings::Apply(RenderCommandList& render_command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    const id<MTLRenderCommandEncoder>& mtl_cmd_encoder = render_command_list.GetNativeCommandEncoder();
    if (m_argument_buffer_range.IsEmpty())
    {
        SetRenderResources(mtl_cmd_encoder, render_command_list.GetProgramBindingsPtr(), apply_behavior);
    }
    else
    {
        SetRenderArgumentBuffers(mtl_cmd_encoder);
    }
}

void ProgramBindings::Apply(ComputeCommandList& compute_command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    const id<MTLComputeCommandEncoder>& mtl_cmd_encoder = compute_command_list.GetNativeCommandEncoder();
    if (m_argument_buffer_range.IsEmpty())
    {
        SetComputeResources(mtl_cmd_encoder, compute_command_list.GetProgramBindingsPtr(), apply_behavior);
    }
    else
    {
        SetComputeArgumentBuffers(mtl_cmd_encoder);
    }
}

} // namespace Methane::Graphics::Metal
