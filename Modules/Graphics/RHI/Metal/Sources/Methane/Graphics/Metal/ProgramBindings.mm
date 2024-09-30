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

#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

using NativeBuffers       = ProgramArgumentBinding::NativeBuffers;
using NativeTextures      = ProgramArgumentBinding::NativeTextures;
using NativeSamplerStates = ProgramArgumentBinding::NativeSamplerStates;
using NativeOffsets       = ProgramArgumentBinding::NativeOffsets;
using CommandType         = ProgramBindings::CommandType;

constexpr ProgramBindings::ApplyBehaviorMask g_constant_once_and_changes_only({
    ProgramBindings::ApplyBehavior::ConstantOnce,
    ProgramBindings::ApplyBehavior::ChangesOnly
});

#ifdef METHANE_LOGGING_ENABLED

static std::string GetNativeResourceUsageName(MTLResourceUsage mtl_resource_usage)
{
    std::stringstream ss;
    if (mtl_resource_usage & MTLResourceUsageRead)
        ss << "Read";

    if (mtl_resource_usage & MTLResourceUsageWrite)
    {
        if (ss.rdbuf()->in_avail() > 0)
            ss << "|";
        ss << "Write";
    }

    return ss.str();
}

static std::string GetNativeRenderStageNames(MTLRenderStages mtl_render_stages)
{
    std::stringstream ss;
    if (mtl_render_stages & MTLRenderStageVertex)
        ss << "Vertex";

    if (mtl_render_stages & MTLRenderStageFragment)
    {
        if (ss.rdbuf()->in_avail() > 0)
            ss << "|";
        ss << "Fragment";
    }

    return ss.str();
}

static std::string GetNativeResourceLabels(const ProgramArgumentBinding::NativeResources& mtl_resources)
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    for(size_t index = 0; index < mtl_resources.size(); ++index)
    {
        const auto& mtl_resource = mtl_resources[index];
        ss << "'" << (mtl_resource.label ? mtl_resource.label.UTF8String : "N/A") << "'";
        if (index < mtl_resources.size() - 1)
            ss << ", ";
    }
    return ss.str();
}

#endif // METHANE_LOGGING_ENABLED

template<CommandType command_type,
         typename TMetalResource,
         typename CommandEncoderType = typename Command<command_type>::EncoderType>
void SetMetalResource(Rhi::ShaderType shader_type,
                      CommandEncoderType mtl_cmd_encoder,
                      __unsafe_unretained TMetalResource mtl_resource,
                      uint32_t arg_index, NSUInteger buffer_offset);

template<CommandType command_type,
         typename TMetalResource,
         typename CommandEncoderType = typename Command<command_type>::EncoderType>
void SetMetalResources(Rhi::ShaderType shader_type,
                       CommandEncoderType mtl_cmd_encoder,
                       const std::vector<__unsafe_unretained TMetalResource>& mtl_resource,
                       uint32_t arg_index, const std::vector<NSUInteger>& buffer_offsets);

template<>
void SetMetalResource<CommandType::Render>(Rhi::ShaderType shader_type,
                                           __unsafe_unretained id<MTLRenderCommandEncoder> mtl_cmd_encoder,
                                           __unsafe_unretained id<MTLBuffer> mtl_buffer,
                                           uint32_t arg_index, NSUInteger buffer_offset)
{
    META_FUNCTION_TASK();
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexBuffer:mtl_buffer
                                      offset:buffer_offset
                                     atIndex:arg_index];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentBuffer:mtl_buffer
                                        offset:buffer_offset
                                       atIndex:arg_index];
            break;

        default:
            META_UNEXPECTED(shader_type);
    }
}

template<>
void SetMetalResources<CommandType::Render>(Rhi::ShaderType shader_type,
                                            __unsafe_unretained id<MTLRenderCommandEncoder> mtl_cmd_encoder,
                                            const NativeBuffers& mtl_buffers,
                                            uint32_t arg_index,
                                            const std::vector<NSUInteger>& buffer_offsets)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_buffers.size());
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexBuffers:mtl_buffers.data()
                                      offsets:buffer_offsets.data()
                                    withRange:args_range];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentBuffers:mtl_buffers.data()
                                        offsets:buffer_offsets.data()
                                      withRange:args_range];
            break;

        default:
            META_UNEXPECTED(shader_type);
    }
}

template<>
void SetMetalResource<CommandType::Render>(Rhi::ShaderType shader_type,
                                           __unsafe_unretained id<MTLRenderCommandEncoder> mtl_cmd_encoder,
                                           __unsafe_unretained id<MTLTexture> mtl_texture,
                                           uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexTexture:mtl_texture
                                      atIndex:arg_index];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentTexture:mtl_texture
                                        atIndex:arg_index];
            break;

        default:
            META_UNEXPECTED(shader_type);
    }
}

template<>
void SetMetalResources<CommandType::Render>(Rhi::ShaderType shader_type,
                                            __unsafe_unretained id<MTLRenderCommandEncoder> mtl_cmd_encoder,
                                            const NativeTextures& mtl_textures,
                                            uint32_t arg_index,
                                            const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_textures.size());
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexTextures:mtl_textures.data()
                                     withRange:args_range];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentTextures:mtl_textures.data()
                                       withRange:args_range];
            break;

        default:
            META_UNEXPECTED(shader_type);
    }
}

template<>
void SetMetalResource<CommandType::Render>(Rhi::ShaderType shader_type,
                                           __unsafe_unretained id<MTLRenderCommandEncoder> mtl_cmd_encoder,
                                           __unsafe_unretained id<MTLSamplerState> mtl_sampler,
                                           uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexSamplerState:mtl_sampler
                                           atIndex:arg_index];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentSamplerState:mtl_sampler
                                             atIndex:arg_index];
            break;

        default:
            META_UNEXPECTED(shader_type);
    }
}

template<>
void SetMetalResources<CommandType::Render>(Rhi::ShaderType shader_type,
                                            __unsafe_unretained id<MTLRenderCommandEncoder> mtl_cmd_encoder,
                                            const NativeSamplerStates& mtl_samplers,
                                            uint32_t arg_index,
                                            const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    const NSRange args_range = NSMakeRange(arg_index, mtl_samplers.size());
    switch (shader_type)
    {
        case Rhi::ShaderType::Vertex:
            [mtl_cmd_encoder setVertexSamplerStates:mtl_samplers.data()
                                          withRange:args_range];
            break;

        case Rhi::ShaderType::Pixel:
            [mtl_cmd_encoder setFragmentSamplerStates:mtl_samplers.data()
                                            withRange:args_range];
            break;

        default:
            META_UNEXPECTED(shader_type);
    }
}

template<>
void SetMetalResource<CommandType::Compute>(Rhi::ShaderType,
                                            __unsafe_unretained id<MTLComputeCommandEncoder> mtl_cmd_encoder,
                                            __unsafe_unretained id<MTLBuffer> mtl_buffer,
                                            uint32_t arg_index, NSUInteger buffer_offset)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setBuffer:mtl_buffer
                        offset:buffer_offset
                       atIndex:arg_index];
}

template<>
void SetMetalResources<CommandType::Compute>(Rhi::ShaderType,
                                             __unsafe_unretained id<MTLComputeCommandEncoder> mtl_cmd_encoder,
                                             const NativeBuffers& mtl_buffers,
                                             uint32_t arg_index,
                                             const std::vector<NSUInteger>& buffer_offsets)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setBuffers:mtl_buffers.data()
                        offsets:buffer_offsets.data()
                      withRange:NSMakeRange(arg_index, mtl_buffers.size())];
}

template<>
void SetMetalResource<CommandType::Compute>(Rhi::ShaderType,
                                            __unsafe_unretained id<MTLComputeCommandEncoder> mtl_cmd_encoder,
                                            __unsafe_unretained id<MTLTexture> mtl_texture,
                                            uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setTexture:mtl_texture
                        atIndex:arg_index];
}

template<>
void SetMetalResources<CommandType::Compute>(Rhi::ShaderType,
                                             __unsafe_unretained id<MTLComputeCommandEncoder> mtl_cmd_encoder,
                                             const NativeTextures& mtl_textures,
                                             uint32_t arg_index, const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setTextures:mtl_textures.data()
                       withRange:NSMakeRange(arg_index, mtl_textures.size())];
}

template<>
void SetMetalResource<CommandType::Compute>(Rhi::ShaderType,
                                            __unsafe_unretained id<MTLComputeCommandEncoder> mtl_cmd_encoder,
                                            __unsafe_unretained id<MTLSamplerState> mtl_sampler,
                                            uint32_t arg_index, NSUInteger)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setSamplerState:mtl_sampler
                             atIndex:arg_index];
}

template<>
void SetMetalResources<CommandType::Compute>(Rhi::ShaderType,
                                             __unsafe_unretained id<MTLComputeCommandEncoder> mtl_cmd_encoder,
                                             const NativeSamplerStates& mtl_samplers,
                                             uint32_t arg_index, const std::vector<NSUInteger>&)
{
    META_FUNCTION_TASK();
    [mtl_cmd_encoder setSamplerStates:mtl_samplers.data()
                            withRange:NSMakeRange(arg_index, mtl_samplers.size())];
}

template<CommandType command_type,
         typename TMetalResource,
         typename CommandEncoderType = typename Command<command_type>::EncoderType>
void SetMetalResourcesForAll(Rhi::ShaderType shader_type,
                             const Rhi::IProgram& program,
                             const CommandEncoderType& mtl_cmd_encoder,
                             const std::vector<TMetalResource>& mtl_resources,
                             uint32_t arg_index, const std::vector<NSUInteger>& offsets = std::vector<NSUInteger>())
{
    META_FUNCTION_TASK();
    if (mtl_resources.empty())
        return;

    if constexpr (command_type == CommandType::Render)
    {
        if (shader_type == Rhi::ShaderType::All)
        {
            if (mtl_resources.size() > 1)
            {
                for (Rhi::ShaderType specific_shader_type: program.GetShaderTypes())
                {
                    SetMetalResources<command_type>(specific_shader_type, mtl_cmd_encoder, mtl_resources,
                                                    arg_index, offsets);
                }
            }
            else
            {
                for (Rhi::ShaderType specific_shader_type: program.GetShaderTypes())
                {
                    SetMetalResource<command_type>(specific_shader_type, mtl_cmd_encoder, mtl_resources.back(),
                                                   arg_index, offsets.empty() ? 0 : offsets.back());
                }
            }
            return;
        }
    }

    if (mtl_resources.size() > 1)
    {
        SetMetalResources<command_type>(shader_type, mtl_cmd_encoder, mtl_resources,
                                        arg_index, offsets);
    }
    else
    {
        SetMetalResource<command_type>(shader_type, mtl_cmd_encoder, mtl_resources.back(),
                                       arg_index, offsets.empty() ? 0 : offsets.back());
    }
}

static void WriteNativeBufferAddresses(const NativeBuffers& native_buffers, const NativeOffsets& offsets, uint64_t* argument_ptr)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(native_buffers.size(), offsets.size());
    for(size_t buffer_index = 0; buffer_index < native_buffers.size(); buffer_index++)
    {
        const auto& native_buffer = native_buffers[buffer_index];
        const uint64_t offset = offsets[buffer_index];
        *argument_ptr = native_buffer.gpuAddress + offset;
        META_LOG("        {}) buffer '{}' address '{}'", buffer_index,
                 native_buffer.label ? native_buffer.label.UTF8String : "N/A",
                 *argument_ptr);
        argument_ptr++;
    }
}

static void WriteNativeTextureIds(const NativeTextures& native_textures, uint64_t* argument_ptr)
{
    META_FUNCTION_TASK();
    for(size_t texture_index = 0; texture_index < native_textures.size(); ++texture_index)
    {
        const auto& native_texture = native_textures[texture_index];
        *argument_ptr = native_texture.gpuResourceID._impl;
        META_LOG("        {}) texture '{}' gpu resource id '{}'", texture_index, native_texture.label.UTF8String, *argument_ptr);
        argument_ptr++;
    }
}

static void WriteNativeSamplerIds(const NativeSamplerStates& native_samplers, uint64_t* argument_ptr)
{
    META_FUNCTION_TASK();
    for(size_t sampler_index = 0; sampler_index < native_samplers.size(); ++sampler_index)
    {
        const auto& native_sampler = native_samplers[sampler_index];
        *argument_ptr = native_sampler.gpuResourceID._impl;
        META_LOG("        {}) sampler gpu resource id '{}'", sampler_index, *argument_ptr);
        argument_ptr++;
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
            META_UNEXPECTED(resource_type);
    }
}

ProgramBindings::ProgramBindings(Program& program, const BindingValueByArgument& binding_value_by_argument,
                                 Data::Index frame_index)
    : Base::ProgramBindings(program, binding_value_by_argument, frame_index)
{
    UpdateUsedResources();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings,
                                 const BindingValueByArgument& replace_resource_views_by_argument,
                                 const Opt<Data::Index>& frame_index)
    : Base::ProgramBindings(other_program_bindings, replace_resource_views_by_argument, frame_index)
{
    UpdateUsedResources();
}

ProgramBindings::~ProgramBindings()
{
    META_FUNCTION_TASK();
    Base::ProgramBindings::RemoveFromDescriptorManager();
}

Ptr<Rhi::IProgramBindings> ProgramBindings::CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument,
                                                       const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<ProgramBindings>(*this, replace_binding_value_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    RetainRootConstantBuffers();
    switch (const Rhi::CommandListType command_list_type = command_list.GetType();
            command_list_type)
    {
        case Rhi::CommandListType::Render:
            Apply<CommandType::Render>(static_cast<RenderCommandList&>(command_list), apply_behavior);
            break;

        case Rhi::CommandListType::Compute:
            Apply<CommandType::Compute>(static_cast<ComputeCommandList&>(command_list), apply_behavior);
            break;

        default:
            META_UNEXPECTED(command_list_type);
    }
}

void ProgramBindings::CompleteInitialization()
{
    META_FUNCTION_TASK();
    const auto& program = GetMetalProgram();
    auto& descriptor_manager = static_cast<DescriptorManager&>(program.GetContext().GetDescriptorManager());

    for(Rhi::ProgramArgumentAccessType access_type : magic_enum::enum_values<Rhi::ProgramArgumentAccessType>())
    {
        if (!m_argument_buffer_initialized_access_types.HasAnyBit(access_type) &&
            WriteArgumentsBufferRange(descriptor_manager, access_type, GetArgumentsRange(access_type)))
        {
             m_argument_buffer_initialized_access_types.SetBitOn(access_type);
        }
    }
}

bool ProgramBindings::WriteArgumentsBufferRange(DescriptorManager& descriptor_manager,
                                                Rhi::ProgramArgumentAccessType access_type,
                                                const ArgumentsRange& args_range)
{
    if (args_range.IsEmpty())
        return false;

    META_LOG("  - Writing program '{}' {} bindings '{}' in arg-buffer range [{}, {}):",
             Base::ProgramBindings::GetProgram().GetName(),
             magic_enum::enum_name(access_type),
             GetName(), args_range.GetStart(), args_range.GetEnd());

    DescriptorManager::ArgumentsBuffer& args_buffer = descriptor_manager.GetArgumentsBuffer(access_type);
    META_CHECK_LESS_OR_EQUAL(args_range.GetEnd(), args_buffer.GetDataSize());

    Data::Byte* arg_data_ptr = args_buffer.GetDataPtr() + args_range.GetStart();
    for (const auto& [program_arg, arg_binding_ptr]: GetArgumentBindings())
    {
        META_CHECK_NOT_NULL(arg_binding_ptr);
        const auto& metal_argument_binding = static_cast<const ArgumentBinding&>(*arg_binding_ptr);
        if (metal_argument_binding.GetSettings().argument.GetAccessorType() != access_type)
            continue;

        for(const auto& [shader_type, arg_offset] : metal_argument_binding.GetMetalSettings().argument_buffer_offset_by_shader_type)
        {
            META_LOG("    - {} shader argument '{}' binding at offset {}:",
                     magic_enum::enum_name(shader_type),
                     metal_argument_binding.GetSettings().argument.GetName(),
                     arg_offset);
            META_CHECK_LESS(arg_offset, args_range.GetLength());
            WriteArgumentBindingResourceIds(
                metal_argument_binding,
                reinterpret_cast<uint64_t*>(arg_data_ptr + arg_offset) // NOSONAR
            );
        }
    }

    return true;
}

const ProgramBindings::ArgumentsRange& ProgramBindings::GetArgumentsRange(Rhi::ProgramArgumentAccessType access_type) const
{
    META_FUNCTION_TASK();
    switch(access_type)
    {
        case Rhi::ProgramArgumentAccessType::Constant:
            return GetMetalProgram().GetConstantArgumentBufferRange();

        case Rhi::ProgramArgumentAccessType::FrameConstant:
            return GetMetalProgram().GetFrameConstantArgumentBufferRange(GetFrameIndex());

        case Rhi::ProgramArgumentAccessType::Mutable:
            return m_mutable_argument_buffer_range;

        default:
            META_UNEXPECTED(access_type);
    }
}

void ProgramBindings::SetMutableArgumentsRange(const ArgumentsRange& mutable_arg_range)
{
    META_FUNCTION_TASK();
    m_mutable_argument_buffer_range = mutable_arg_range;
}

bool ProgramBindings::IsUsingNativeResource(__unsafe_unretained id<MTLResource> mtl_resource) const
{
    return m_mtl_used_resources.count(mtl_resource);
}

Program& ProgramBindings::GetMetalProgram() const
{
    META_FUNCTION_TASK();
    return dynamic_cast<Program&>(Base::ProgramBindings::GetProgram());
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

template<CommandType command_type, typename CommandEncoderType>
void ProgramBindings::SetMetalResources(const CommandEncoderType& mtl_cmd_encoder,
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
                    SetMetalResourcesForAll<command_type>(settings.argument.GetShaderType(), program, mtl_cmd_encoder,
                                                          argument_binding.GetNativeBuffers(),
                                                          settings.argument_index,
                                                          argument_binding.GetBufferOffsets());
                    break;

                case Rhi::ResourceType::Texture:
                    SetMetalResourcesForAll<command_type>(settings.argument.GetShaderType(), program, mtl_cmd_encoder,
                                                          argument_binding.GetNativeTextures(),
                                                          settings.argument_index);
                    break;

                case Rhi::ResourceType::Sampler:
                    SetMetalResourcesForAll<command_type>(settings.argument.GetShaderType(), program, mtl_cmd_encoder,
                                                          argument_binding.GetNativeSamplerStates(),
                                                          settings.argument_index);
                    break;

                default:
                    META_UNEXPECTED(settings.resource_type);
            }
        }
    );
}

template<CommandType command_type, typename CommandEncoderType>
void ProgramBindings::SetMetalArgumentBuffers(const CommandEncoderType& mtl_cmd_encoder,
                                              const Base::ProgramBindings* applied_program_bindings_ptr,
                                              ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    META_LOG("  - Apply {} program binding '{}' with argument buffers:", magic_enum::enum_name<command_type>(), GetName());

    const auto& program = GetMetalProgram();
    auto& descriptor_manager = static_cast<DescriptorManager&>(program.GetContext().GetDescriptorManager());
    std::array<Data::Size, magic_enum::enum_count<Rhi::ProgramArgumentAccessType>()> arg_layout_offset_by_buffer;
    std::fill(arg_layout_offset_by_buffer.begin(), arg_layout_offset_by_buffer.end(), 0U);

    for(const Program::ShaderArgumentBufferLayout& arg_layout : program.GetShaderArgumentBufferLayouts())
    {
        // Skip setup of constant argument bindings, when program bindings were already applied to command list:
        if (apply_behavior.HasAnyBits(g_constant_once_and_changes_only) &&
            arg_layout.access_type != Rhi::ProgramArgumentAccessType::Mutable &&
            applied_program_bindings_ptr)
            continue;

        const DescriptorManager::ArgumentsBuffer& args_buffer = descriptor_manager.GetArgumentsBuffer(arg_layout.access_type);
        const auto buffer_ptr = static_cast<const Buffer*>(args_buffer.GetBuffer());
        META_CHECK_NOT_NULL_DESCR(buffer_ptr, "{} argument buffer is not initialized in Descriptor Manager!",
                                      magic_enum::enum_name(arg_layout.access_type));

        const id<MTLBuffer>& mtl_argument_buffer = buffer_ptr->GetNativeBuffer();
        const ArgumentsRange& args_range = GetArgumentsRange(arg_layout.access_type);
        const auto arg_buffer_index = static_cast<NSUInteger>(arg_layout.access_type);
        Data::Size& arg_layout_offset = arg_layout_offset_by_buffer[arg_buffer_index];
        const Data::Index arg_buffer_offset = args_range.GetStart() + arg_layout_offset;
        META_CHECK_LESS_DESCR(arg_buffer_offset, args_range.GetEnd(), "invalid offset in argument buffer");
        META_LOG("    - {} shader {} argument buffer [{}] at offset {}",
                 magic_enum::enum_name(arg_layout.shader_type),
                 magic_enum::enum_name(arg_layout.access_type),
                 arg_buffer_index,
                 arg_buffer_offset);

        if constexpr (command_type == CommandType::Render)
        {
            switch (arg_layout.shader_type)
            {
                case Rhi::ShaderType::Vertex:
                    [mtl_cmd_encoder setVertexBuffer:mtl_argument_buffer
                                              offset:arg_buffer_offset
                                             atIndex:arg_buffer_index];
                    break;

                case Rhi::ShaderType::Pixel:
                    [mtl_cmd_encoder setFragmentBuffer:mtl_argument_buffer
                                                offset:arg_buffer_offset
                                               atIndex:arg_buffer_index];
                    break;

                default:
                    META_UNEXPECTED(arg_layout.shader_type);
            }
        }
        if constexpr (command_type == CommandType::Compute)
        {
            [mtl_cmd_encoder setBuffer:mtl_argument_buffer
                                offset:arg_buffer_offset
                               atIndex:arg_buffer_index];
        }

        arg_layout_offset += arg_layout.data_size;
    }
}

template<CommandType command_type, typename CommandEncoderType>
void ProgramBindings::UseMetalResources(const CommandEncoderType& mtl_cmd_encoder,
                                        const Base::ProgramBindings* applied_program_bindings_ptr) const
{
    META_FUNCTION_TASK();
    [[maybe_unused]] bool is_first_use_resources = true;
    const NativeResourcesByUsage changed_resources_by_usage = GetChangedResourcesByUsage(applied_program_bindings_ptr);
    for(const auto& [mtl_usage_and_stage, mtl_resources] : changed_resources_by_usage)
    {
#ifdef METHANE_LOGGING_ENABLED
        if (is_first_use_resources)
        {
            META_LOG("  - Make resident of render program binding '{}' resources:", GetName());
            is_first_use_resources = false;
        }
#endif

        META_LOG("    - {} of {} shader resources: {}.",
                 GetNativeResourceUsageName(mtl_usage_and_stage.first),
                 GetNativeRenderStageNames(mtl_usage_and_stage.second),
                 GetNativeResourceLabels(mtl_resources));

        if constexpr (command_type == CommandType::Render)
        {
            [mtl_cmd_encoder useResources:mtl_resources.data()
                                    count:mtl_resources.size()
                                    usage:mtl_usage_and_stage.first
                                   stages:mtl_usage_and_stage.second];
        }
        if constexpr (command_type == CommandType::Compute)
        {
            [mtl_cmd_encoder useResources:mtl_resources.data()
                                    count:mtl_resources.size()
                                    usage:mtl_usage_and_stage.first];
        }
    }
}

void ProgramBindings::UpdateUsedResources()
{
    META_FUNCTION_TASK();
    m_mtl_used_resources.clear();
    for(const auto& [argument, argument_binding_ptr] : GetArgumentBindings())
    {
        const auto& argument_binding = static_cast<const ArgumentBinding&>(*argument_binding_ptr);
        const ProgramArgumentBinding::NativeResources& argument_resources = argument_binding.GetNativeResources();
        std::copy(argument_resources.begin(), argument_resources.end(),
                  std::inserter(m_mtl_used_resources, m_mtl_used_resources.begin()));
    }
}

void ProgramBindings::UpdateArgumentBuffer(const IArgumentBinding& changed_arg_binding)
{
    META_FUNCTION_TASK();
    const Rhi::ProgramArgumentAccessType access_type = changed_arg_binding.GetSettings().argument.GetAccessorType();
    if (m_mtl_used_resources.empty() || access_type == Rhi::ProgramArgumentAccessType::Mutable)
    {
        UpdateUsedResources();
    }

    const auto& program = GetMetalProgram();
    if (auto& descriptor_manager = static_cast<DescriptorManager&>(program.GetContext().GetDescriptorManager());
        WriteArgumentsBufferRange(descriptor_manager, access_type, GetArgumentsRange(access_type)))
    {
        // Update argument buffer data on GPU:
        const Base::Context& context = GetMetalProgram().GetContext();
        static_cast<DescriptorManager&>(context.GetDescriptorManager()).GetArgumentsBuffer(access_type).Update();
    }
}

ProgramBindings::NativeResourcesByUsage ProgramBindings::GetChangedResourcesByUsage(
                                                         const Base::ProgramBindings* applied_program_bindings_ptr) const
{
    META_FUNCTION_TASK();
    NativeResourcesByUsage resources_by_usage;
    const ProgramBindings* metal_applied_program_bindings_ptr = static_cast<const ProgramBindings*>(applied_program_bindings_ptr);
    for (const auto& [program_arg, arg_binding_ptr]: GetArgumentBindings())
    {
        const auto& metal_argument_binding = static_cast<const ArgumentBinding&>(*arg_binding_ptr);
        const ProgramArgumentBinding::NativeResources& argument_resources = metal_argument_binding.GetNativeResources();
        if (argument_resources.empty())
            continue;

        const NativeResourceUsageAndStage usage_and_stage(metal_argument_binding.GetNativeResourceUsage(),
                                                          metal_argument_binding.GetNativeRenderStages());

        if (metal_applied_program_bindings_ptr)
        {
            if (!arg_binding_ptr->GetSettings().argument.IsConstant())
                for (const auto& mtl_resource: argument_resources)
                    if (!metal_applied_program_bindings_ptr->IsUsingNativeResource(mtl_resource))
                        resources_by_usage[usage_and_stage].push_back(mtl_resource);
        }
        else
        {
            std::move(argument_resources.begin(), argument_resources.end(),
                      std::back_inserter(resources_by_usage[usage_and_stage]));
        }
    }
    return resources_by_usage;
}

template<CommandType command_type, typename CommandListType>
void ProgramBindings::Apply(CommandListType& command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    const auto& mtl_cmd_encoder = command_list.GetNativeCommandEncoder();
    const Base::ProgramBindings* applied_program_bindings_ptr = command_list.GetProgramBindingsPtr();
    if (static_cast<bool>(m_argument_buffer_initialized_access_types))
    {
        UseMetalResources<command_type>(mtl_cmd_encoder, applied_program_bindings_ptr);
        SetMetalArgumentBuffers<command_type>(mtl_cmd_encoder, applied_program_bindings_ptr, apply_behavior);
    }
    else
    {
        SetMetalResources<command_type>(mtl_cmd_encoder, applied_program_bindings_ptr, apply_behavior);
    }
}

void ProgramBindings::OnProgramArgumentBindingResourceViewsChanged(const IArgumentBinding& arg_binding,
                                                                   const Rhi::ResourceViews&,
                                                                   const Rhi::ResourceViews&)
{
    META_FUNCTION_TASK();
    UpdateArgumentBuffer(arg_binding);
}

void ProgramBindings::OnProgramArgumentBindingRootConstantChanged(const IArgumentBinding& arg_binding,
                                                                  const Rhi::RootConstant&)
{
    META_FUNCTION_TASK();
    UpdateArgumentBuffer(arg_binding);
}

} // namespace Methane::Graphics::Metal
