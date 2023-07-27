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

FILE: Methane/Graphics/Vulkan/Shader.cpp
Vulkan implementation of the shader interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Shader.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/ProgramBindings.h>

#include <Methane/Data/IProvider.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

namespace Methane::Graphics::Vulkan
{

static vk::VertexInputRate ConvertInputBufferLayoutStepTypeToVertexInputRate(Rhi::IProgram::InputBufferLayout::StepType step_type)
{
    META_FUNCTION_TASK();
    using StepType = Rhi::IProgram::InputBufferLayout::StepType;
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

static Rhi::IResource::Type ConvertDescriptorTypeToResourceType(vk::DescriptorType vk_descriptor_type)
{
    META_FUNCTION_TASK();
    switch(vk_descriptor_type)
    {
    case vk::DescriptorType::eUniformBuffer:
    case vk::DescriptorType::eStorageBuffer:
        return Rhi::IResource::Type::Buffer;

    case vk::DescriptorType::eStorageImage:
    case vk::DescriptorType::eSampledImage:
        return Rhi::IResource::Type::Texture;

    case vk::DescriptorType::eSampler:
        return Rhi::IResource::Type::Sampler;

    default:
        META_UNEXPECTED_ARG_RETURN(vk_descriptor_type, Rhi::IResource::Type::Buffer);
    }
}

static vk::DescriptorType UpdateDescriptorType(vk::DescriptorType vk_shader_descriptor_type, const Rhi::ProgramArgumentAccessor& argument_accessor)
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

static void AddSpirvResourcesToArgumentBindings(const spirv_cross::Compiler& spirv_compiler,
                                                const spirv_cross::SmallVector<spirv_cross::Resource>& spirv_resources,
                                                const vk::DescriptorType vk_descriptor_type,
                                                const Rhi::ProgramArgumentAccessors& argument_accessors,
                                                const Shader& shader,
                                                Ptrs<Base::ProgramArgumentBinding>& argument_bindings)
{
    META_FUNCTION_TASK();
    if (spirv_resources.begin() == spirv_resources.end())
        return;

    const Rhi::IResource::Type resource_type = ConvertDescriptorTypeToResourceType(vk_descriptor_type);\
    const Rhi::ShaderType shader_type = shader.GetType();

    for (const spirv_cross::Resource& resource : spirv_resources)
    {
        const Rhi::IProgram::Argument shader_argument(shader_type, shader.GetCachedArgName(spirv_compiler.get_name(resource.id)));
        const auto argument_acc_it = Rhi::IProgram::FindArgumentAccessor(argument_accessors, shader_argument);
        const Rhi::ProgramArgumentAccessor argument_acc = argument_acc_it == argument_accessors.end()
                                                   ? Rhi::ProgramArgumentAccessor(shader_argument)
                                                   : *argument_acc_it;

        const spirv_cross::SPIRType& spirv_type = spirv_compiler.get_type(resource.type_id);
        const uint32_t array_size = GetArraySize(spirv_type);

        ProgramBindings::ArgumentBinding::ByteCodeMap byte_code_map{ shader_type };
        META_CHECK_ARG_TRUE(spirv_compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationDescriptorSet, byte_code_map.descriptor_set_offset));
        META_CHECK_ARG_TRUE(spirv_compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationBinding, byte_code_map.binding_offset));

        argument_bindings.push_back(std::make_shared<ProgramBindings::ArgumentBinding>(
            shader.GetContext(),
            ProgramArgumentBindingSettings
            {
                Rhi::ProgramArgumentBindingSettings
                {
                    argument_acc,
                    resource_type,
                    array_size
                },
                UpdateDescriptorType(vk_descriptor_type, argument_acc),
                { std::move(byte_code_map) }
            }
        ));

        META_LOG("  - '{}' with descriptor type {}, array size {};",
                 shader_argument.GetName(),
                 vk::to_string(vk_descriptor_type),
                 array_size);
    }
}

Shader::Shader(Rhi::ShaderType shader_type, const Base::Context& context, const Settings& settings)
    : Base::Shader(shader_type, context, settings)
    , m_vk_context(dynamic_cast<const IContext&>(context))
    , m_byte_code_chunk(settings.data_provider.GetData(fmt::format("{}.spirv", GetCompiledEntryFunctionName(settings))))
{ }

Shader::~Shader() = default;

Ptrs<Base::ProgramArgumentBinding> Shader::GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const
{
    META_FUNCTION_TASK();
    const Rhi::IShader::Settings& shader_settings = GetSettings();
    META_UNUSED(shader_settings);

    META_LOG("{} shader '{}' ({}) with argument bindings:",
             magic_enum::enum_name(GetType()),
             shader_settings.entry_function.function_name,
             Rhi::ShaderMacroDefinition::ToString(shader_settings.compile_definitions));

    Ptrs<Base::ProgramArgumentBinding> argument_bindings;
    const spirv_cross::Compiler& spirv_compiler = GetNativeCompiler();
    const auto add_spirv_resources_to_argument_bindings = [this, &spirv_compiler, &argument_accessors, &argument_bindings]
                                                          (const spirv_cross::SmallVector<spirv_cross::Resource>& spirv_resources,
                                                           const vk::DescriptorType vk_descriptor_type)
    {
        AddSpirvResourcesToArgumentBindings(spirv_compiler, spirv_resources, vk_descriptor_type, argument_accessors, *this, argument_bindings);
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

    if (argument_bindings.empty())
    {
        META_LOG("  - No argument bindings.");
    }

    return argument_bindings;
}

const vk::ShaderModule& Shader::GetNativeModule() const
{
    META_FUNCTION_TASK();
    if (!m_vk_unique_module)
    {
        m_vk_unique_module = m_vk_context.GetVulkanDevice().GetNativeDevice().createShaderModuleUnique(
            vk::ShaderModuleCreateInfo(
                vk::ShaderModuleCreateFlags{},
                m_byte_code_chunk.GetDataSize(),
                m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>())
        );
    }
    return m_vk_unique_module.get();
}

const spirv_cross::Compiler& Shader::GetNativeCompiler() const
{
    META_FUNCTION_TASK();
    if (m_spirv_compiler_ptr)
        return *m_spirv_compiler_ptr;

    m_spirv_compiler_ptr = std::make_unique<spirv_cross::Compiler>(m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>(),
                                                                   m_byte_code_chunk.GetDataSize<uint32_t>());
    return *m_spirv_compiler_ptr;
}

vk::PipelineShaderStageCreateInfo Shader::GetNativeStageCreateInfo() const
{
    META_FUNCTION_TASK();
    return vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags{},
        ConvertTypeToStageFlagBits(GetType()),
        GetNativeModule(),
        GetSettings().entry_function.function_name.c_str()
    );
}

vk::PipelineVertexInputStateCreateInfo Shader::GetNativeVertexInputStateCreateInfo(const Program& program)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetType(), Rhi::ShaderType::Vertex);
    std::lock_guard lock(m_mutex);

    if (!m_vertex_input_initialized)
        InitializeVertexInputDescriptions(program);

    return vk::PipelineVertexInputStateCreateInfo(
        vk::PipelineVertexInputStateCreateFlags{},
        m_vertex_input_binding_descriptions,
        m_vertex_input_attribute_descriptions
    );
}

Data::MutableChunk& Shader::GetMutableByteCode() noexcept
{
    META_FUNCTION_TASK();
    m_vk_unique_module.reset();
    m_spirv_compiler_ptr.reset();
    return m_byte_code_chunk;
}

void Shader::InitializeVertexInputDescriptions(const Program& program)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetType(), Rhi::ShaderType::Vertex);
    META_CHECK_ARG_FALSE_DESCR(m_vertex_input_initialized, "vertex input descriptions are already initialized");

    const Rhi::IShader::Settings              & shader_settings      = GetSettings();
    const Base::Program::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
    m_vertex_input_binding_descriptions.reserve(input_buffer_layouts.size());

    uint32_t input_buffer_index = 0U;
    for(const Rhi::IProgram::InputBufferLayout& input_buffer_layout : input_buffer_layouts)
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
           << "' (" << Rhi::ShaderMacroDefinition::ToString(shader_settings.compile_definitions)
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

vk::ShaderStageFlagBits Shader::ConvertTypeToStageFlagBits(Rhi::ShaderType shader_type)
{
    META_FUNCTION_TASK();
    switch(shader_type)
    {
    case Rhi::ShaderType::All:     return vk::ShaderStageFlagBits::eAll;
    case Rhi::ShaderType::Vertex:  return vk::ShaderStageFlagBits::eVertex;
    case Rhi::ShaderType::Pixel:   return vk::ShaderStageFlagBits::eFragment;
    case Rhi::ShaderType::Compute: return vk::ShaderStageFlagBits::eCompute;
    default: META_UNEXPECTED_ARG_RETURN(shader_type, vk::ShaderStageFlagBits::eAll);
    }
}

} // namespace Methane::Graphics::Vulkan
