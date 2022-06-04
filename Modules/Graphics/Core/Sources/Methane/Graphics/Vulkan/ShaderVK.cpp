/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ShaderVK.cpp
Vulkan implementation of the shader interface.

******************************************************************************/

#include "ShaderVK.h"
#include "ProgramVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"
#include "ProgramBindingsVK.h"

#include <Methane/Data/Provider.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

namespace Methane::Graphics
{

static vk::VertexInputRate ConvertInputBufferLayoutStepTypeToVertexInputRate(Program::InputBufferLayout::StepType step_type)
{
    META_FUNCTION_TASK();
    using StepType = Program::InputBufferLayout::StepType;
    switch(step_type)
    {
    case StepType::PerVertex:   return vk::VertexInputRate::eVertex;
    case StepType::PerInstance: return vk::VertexInputRate::eInstance;
    default:                    META_UNEXPECTED_ARG_RETURN(step_type, vk::VertexInputRate::eVertex);
    }
}

static vk::Format GetFloatVectorFormat(uint32_t vector_size)
{
    META_FUNCTION_TASK();
    switch (vector_size)
    {
    case 1: return vk::Format::eR32Sfloat;
    case 2: return vk::Format::eR32G32Sfloat;
    case 3: return vk::Format::eR32G32B32Sfloat;
    case 4: return vk::Format::eR32G32B32A32Sfloat;
    default: META_UNEXPECTED_ARG_RETURN(vector_size, vk::Format::eUndefined);
    }
}

static vk::Format GetSignedIntegerVectorFormat(uint32_t vector_size)
{
    META_FUNCTION_TASK();
    switch (vector_size)
    {
    case 1: return vk::Format::eR32Sint;
    case 2: return vk::Format::eR32G32Sint;
    case 3: return vk::Format::eR32G32B32Sint;
    case 4: return vk::Format::eR32G32B32A32Sint;
    default: META_UNEXPECTED_ARG_RETURN(vector_size, vk::Format::eUndefined);
    }
}

static vk::Format GetUnsignedIntegerVectorFormat(uint32_t vector_size)
{
    META_FUNCTION_TASK();
    switch (vector_size)
    {
    case 1: return vk::Format::eR32Uint;
    case 2: return vk::Format::eR32G32Uint;
    case 3: return vk::Format::eR32G32B32Uint;
    case 4: return vk::Format::eR32G32B32A32Uint;
    default: META_UNEXPECTED_ARG_RETURN(vector_size, vk::Format::eUndefined);
    }
}

static vk::Format GetVertexAttributeFormatFromSpirvType(const spirv_cross::SPIRType& attribute_type)
{
    META_FUNCTION_TASK();
    switch(attribute_type.basetype)
    {
    case spirv_cross::SPIRType::Float: return GetFloatVectorFormat(attribute_type.vecsize);
    case spirv_cross::SPIRType::UInt:  return GetSignedIntegerVectorFormat(attribute_type.vecsize);
    case spirv_cross::SPIRType::Int:   return GetUnsignedIntegerVectorFormat(attribute_type.vecsize);
    default:                           META_UNEXPECTED_ARG_RETURN(attribute_type.basetype, vk::Format::eUndefined);
    }
}

static uint32_t GetArraySize(const spirv_cross::SPIRType& resource_type) noexcept
{
    META_FUNCTION_TASK();
    if (resource_type.array.empty())
        return 1;

    return resource_type.array.front()
           ? resource_type.array.front()
           : std::numeric_limits<uint32_t>::max();
}

static Resource::Type ConvertDescriptorTypeToResourceType(vk::DescriptorType vk_descriptor_type)
{
    META_FUNCTION_TASK();
    switch(vk_descriptor_type)
    {
    case vk::DescriptorType::eUniformBuffer:
    case vk::DescriptorType::eStorageBuffer:
        return Resource::Type::Buffer;

    case vk::DescriptorType::eStorageImage:
    case vk::DescriptorType::eSampledImage:
        return Resource::Type::Texture;

    case vk::DescriptorType::eSampler:
        return Resource::Type::Sampler;

    default:
        META_UNEXPECTED_ARG_RETURN(vk_descriptor_type, Resource::Type::Buffer);
    }
}

Ptr<Shader> Shader::Create(Shader::Type shader_type, const Context& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ShaderVK>(shader_type, dynamic_cast<const ContextBase&>(context), settings);
}

ShaderVK::ShaderVK(Shader::Type shader_type, const ContextBase& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
    , m_byte_code_chunk(settings.data_provider.GetData(fmt::format("{}.spirv", GetCompiledEntryFunctionName(settings))))
{
    META_FUNCTION_TASK();
}

static vk::DescriptorType UpdateDescriptorType(vk::DescriptorType vk_shader_descriptor_type, const Program::ArgumentAccessor& argument_accessor)
{
    META_FUNCTION_TASK();
    if (!argument_accessor.IsAddressable())
        return vk_shader_descriptor_type;

    switch(vk_shader_descriptor_type)
    {
    case vk::DescriptorType::eUniformBuffer: return vk::DescriptorType::eUniformBufferDynamic;
    case vk::DescriptorType::eStorageBuffer: return vk::DescriptorType::eStorageBufferDynamic;
    default: META_UNEXPECTED_ARG_DESCR_RETURN(vk_shader_descriptor_type, vk_shader_descriptor_type,
                                              "addressable arguments support only Uniform or Storage buffers");
    }
}

ShaderBase::ArgumentBindings ShaderVK::GetArgumentBindings(const Program::ArgumentAccessors& argument_accessors) const
{
    META_FUNCTION_TASK();
    const spirv_cross::Compiler& spirv_compiler = GetNativeCompiler();
    const Shader::Type shader_type = GetType();
    const Shader::Settings& shader_settings = GetSettings();
    ArgumentBindings argument_bindings;

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << magic_enum::enum_name(GetType())
           << " shader '" << shader_settings.entry_function.function_name
           << "' (" << Shader::ConvertMacroDefinitionsToString(shader_settings.compile_definitions)
           << ") with argument bindings:" << std::endl;
#else
    META_UNUSED(shader_settings);
#endif

    const auto add_spirv_resources_to_argument_bindings = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& spirv_resources,
                                                              const vk::DescriptorType vk_descriptor_type)
    {
        if (spirv_resources.begin() == spirv_resources.end())
            return;

        const Resource::Type resource_type = ConvertDescriptorTypeToResourceType(vk_descriptor_type);
        for (const spirv_cross::Resource& resource : spirv_resources)
        {
            const Program::Argument shader_argument(shader_type, ShaderBase::GetCachedArgName(spirv_compiler.get_name(resource.id)));
            const auto argument_acc_it = Program::FindArgumentAccessor(argument_accessors, shader_argument);
            const Program::ArgumentAccessor argument_acc = argument_acc_it == argument_accessors.end()
                                                         ? Program::ArgumentAccessor(shader_argument)
                                                         : *argument_acc_it;
            
            const spirv_cross::SPIRType& spirv_type = spirv_compiler.get_type(resource.type_id);
            const uint32_t array_size = GetArraySize(spirv_type);

            ProgramBindingsVK::ArgumentBindingVK::ByteCodeMap byte_code_map{ shader_type };
            META_CHECK_ARG_TRUE(spirv_compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationDescriptorSet, byte_code_map.descriptor_set_offset));
            META_CHECK_ARG_TRUE(spirv_compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationBinding,       byte_code_map.binding_offset));

            argument_bindings.push_back(std::make_shared<ProgramBindingsVK::ArgumentBindingVK>(
                GetContext(),
                ProgramBindingsVK::ArgumentBindingVK::SettingsVK
                {
                    ProgramBindings::ArgumentBinding::Settings
                    {
                        argument_acc,
                        resource_type,
                        array_size
                    },
                    UpdateDescriptorType(vk_descriptor_type, argument_acc),
                    { std::move(byte_code_map) }
                }
            ));

#ifdef METHANE_LOGGING_ENABLED
            log_ss << "  - '" << shader_argument.GetName()
                   << "' with descriptor type " << vk::to_string(vk_descriptor_type)
                   << ", array size " << array_size
                   << ";" << std::endl;
#endif
        }
    };

    // Get only resources that are statically used in SPIRV-code (skip all resources that are never accessed by the shader)
    const spirv_cross::ShaderResources spirv_resources = spirv_compiler.get_shader_resources(spirv_compiler.get_active_interface_variables());

    add_spirv_resources_to_argument_bindings(spirv_resources.uniform_buffers,   vk::DescriptorType::eUniformBuffer);
    add_spirv_resources_to_argument_bindings(spirv_resources.storage_buffers,   vk::DescriptorType::eStorageBuffer);
    add_spirv_resources_to_argument_bindings(spirv_resources.storage_images,    vk::DescriptorType::eStorageImage);
    add_spirv_resources_to_argument_bindings(spirv_resources.sampled_images,    vk::DescriptorType::eCombinedImageSampler);
    add_spirv_resources_to_argument_bindings(spirv_resources.separate_images,   vk::DescriptorType::eSampledImage);
    add_spirv_resources_to_argument_bindings(spirv_resources.separate_samplers, vk::DescriptorType::eSampler);
    // TODO: add support for spirv_resources.atomic_counters, vk::DescriptorType::eMutableVALVE

#ifdef METHANE_LOGGING_ENABLED
    if (argument_bindings.empty())
        log_ss << "  - No argument bindings." << std::endl;
#endif

    META_LOG("{}", log_ss.str());

    return argument_bindings;
}

const vk::ShaderModule& ShaderVK::GetNativeModule() const
{
    META_FUNCTION_TASK();
    if (!m_vk_unique_module)
    {
        m_vk_unique_module = GetContextVK().GetDeviceVK().GetNativeDevice().createShaderModuleUnique(
            vk::ShaderModuleCreateInfo(
                vk::ShaderModuleCreateFlags{},
                m_byte_code_chunk.GetDataSize(),
                m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>())
        );
    }
    return m_vk_unique_module.get();
}

const spirv_cross::Compiler& ShaderVK::GetNativeCompiler() const
{
    META_FUNCTION_TASK();
    if (m_spirv_compiler_ptr)
        return *m_spirv_compiler_ptr;

    m_spirv_compiler_ptr = std::make_unique<spirv_cross::Compiler>(m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>(),
                                                                   m_byte_code_chunk.GetDataSize<uint32_t>());
    return *m_spirv_compiler_ptr;
}

vk::PipelineShaderStageCreateInfo ShaderVK::GetNativeStageCreateInfo() const
{
    META_FUNCTION_TASK();
    return vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags{},
        ConvertTypeToStageFlagBits(GetType()),
        GetNativeModule(),
        GetSettings().entry_function.function_name.c_str()
    );
}

vk::PipelineVertexInputStateCreateInfo ShaderVK::GetNativeVertexInputStateCreateInfo(const ProgramVK& program)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetType(), Shader::Type::Vertex);
    if (!m_vertex_input_initialized)
        InitializeVertexInputDescriptions(program);

    return vk::PipelineVertexInputStateCreateInfo(
        vk::PipelineVertexInputStateCreateFlags{},
        m_vertex_input_binding_descriptions,
        m_vertex_input_attribute_descriptions
    );
}

Data::MutableChunk& ShaderVK::GetMutableByteCode() noexcept
{
    META_FUNCTION_TASK();
    m_vk_unique_module.reset();
    m_spirv_compiler_ptr.reset();
    return m_byte_code_chunk;
}

void ShaderVK::InitializeVertexInputDescriptions(const ProgramVK& program)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetType(), Shader::Type::Vertex);
    META_CHECK_ARG_FALSE_DESCR(m_vertex_input_initialized, "vertex input descriptions are already initialized");

    const Shader::Settings& shader_settings = GetSettings();
    const ProgramBase::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
    m_vertex_input_binding_descriptions.reserve(input_buffer_layouts.size());

    uint32_t input_buffer_index = 0U;
    for(const Program::InputBufferLayout& input_buffer_layout : input_buffer_layouts)
    {
        m_vertex_input_binding_descriptions.emplace_back(
            input_buffer_index,
            0U, // stride is auto calculated by vertex attributes
            ConvertInputBufferLayoutStepTypeToVertexInputRate(input_buffer_layout.step_type)
        );
        input_buffer_index++;
    }

    const spirv_cross::Compiler& spirv_compiler = GetNativeCompiler();
    const spirv_cross::ShaderResources shader_resources = spirv_compiler.get_shader_resources();

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << magic_enum::enum_name(GetType())
           << " shader '" << shader_settings.entry_function.function_name
           << "' (" << Shader::ConvertMacroDefinitionsToString(shader_settings.compile_definitions)
           << ") input layout:" << std::endl;
    if (shader_resources.stage_inputs.empty())
        log_ss << " - No stage inputs." << std::endl;
#else
    META_UNUSED(shader_settings);
#endif

    m_vertex_input_attribute_descriptions.reserve(shader_resources.stage_inputs.size());
    for(const spirv_cross::Resource& input_resource : shader_resources.stage_inputs)
    {
        const bool has_semantic = spirv_compiler.has_decoration(input_resource.id, spv::DecorationHlslSemanticGOOGLE);
        const bool has_location = spirv_compiler.has_decoration(input_resource.id, spv::DecorationLocation);
        META_CHECK_ARG_TRUE(has_semantic && has_location);

        const std::string&           semantic_name    = spirv_compiler.get_decoration_string(input_resource.id, spv::DecorationHlslSemanticGOOGLE);
        const uint32_t               input_location   = spirv_compiler.get_decoration(input_resource.id, spv::DecorationLocation);
        const spirv_cross::SPIRType& attribute_type   = spirv_compiler.get_type(input_resource.base_type_id);
        const vk::Format             attribute_format = GetVertexAttributeFormatFromSpirvType(attribute_type);

        const uint32_t buffer_index = GetProgramInputBufferIndexByArgumentSemantic(program, semantic_name);
        META_CHECK_ARG_LESS(buffer_index, m_vertex_input_binding_descriptions.size());
        vk::VertexInputBindingDescription& input_binding_desc = m_vertex_input_binding_descriptions[buffer_index];

        m_vertex_input_attribute_descriptions.emplace_back(
            input_location,
            buffer_index,
            attribute_format,
            input_binding_desc.stride
        );

#ifdef METHANE_LOGGING_ENABLED
        log_ss << "  - Input semantic name '" << semantic_name
               << "' location " << input_location
               << " buffer " << buffer_index
               << " binding " << input_binding_desc.binding
               << " with attribute format " << vk::to_string(attribute_format)
               << ";" << std::endl;
#endif

        // Tight packing of attributes in vertex buffer is assumed
        input_binding_desc.stride += attribute_type.vecsize * 4;
    }

    META_LOG("{}", log_ss.str());

    m_vertex_input_initialized = true;
}

const IContextVK& ShaderVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContext());
}

vk::ShaderStageFlagBits ShaderVK::ConvertTypeToStageFlagBits(Shader::Type shader_type)
{
    META_FUNCTION_TASK();
    switch(shader_type)
    {
    case Shader::Type::All:    return vk::ShaderStageFlagBits::eAll;
    case Shader::Type::Vertex: return vk::ShaderStageFlagBits::eVertex;
    case Shader::Type::Pixel:  return vk::ShaderStageFlagBits::eFragment;
    default:                   META_UNEXPECTED_ARG_RETURN(shader_type, vk::ShaderStageFlagBits::eAll);
    }
}

} // namespace Methane::Graphics
