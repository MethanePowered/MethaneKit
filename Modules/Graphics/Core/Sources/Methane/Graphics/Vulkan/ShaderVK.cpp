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

FILE: Methane/Graphics/Vulkan/ShaderVK.cpp
Vulkan implementation of the shader interface.

******************************************************************************/

#include "ShaderVK.h"
#include "ProgramVK.h"
#include "ContextVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

namespace Methane::Graphics
{

static vk::ShaderStageFlagBits ConvertShaderTypeToStageFlagBits(Shader::Type shader_type)
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

Ptr<Shader> Shader::Create(Shader::Type shader_type, const Context& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ShaderVK>(shader_type, dynamic_cast<const ContextBase&>(context), settings);
}

ShaderVK::ShaderVK(Shader::Type shader_type, const ContextBase& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
    , m_byte_code_chunk_ptr(std::make_unique<Data::Chunk>(settings.data_provider.GetData(fmt::format("{}.spirv", GetCompiledEntryFunctionName(settings)))))
    , m_vk_unique_module(GetContextVK().GetDeviceVK().GetNativeDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags{}, m_byte_code_chunk_ptr->GetDataSize(), m_byte_code_chunk_ptr->GetDataPtr<uint32_t>())))
{
    META_FUNCTION_TASK();
}

ShaderBase::ArgumentBindings ShaderVK::GetArgumentBindings(const Program::ArgumentAccessors&) const
{
    META_FUNCTION_TASK();
    ArgumentBindings argument_bindings;
    return argument_bindings;
}

const spirv_cross::Compiler& ShaderVK::GetNativeCompiler() const
{
    META_FUNCTION_TASK();
    if (m_spirv_compiler_ptr)
        return *m_spirv_compiler_ptr;

    META_CHECK_ARG_NOT_NULL(m_byte_code_chunk_ptr);
    m_spirv_compiler_ptr = std::make_unique<spirv_cross::Compiler>(m_byte_code_chunk_ptr->GetDataPtr<uint32_t>(), m_byte_code_chunk_ptr->GetDataSize<uint32_t>());
    return *m_spirv_compiler_ptr;
}

vk::PipelineShaderStageCreateInfo ShaderVK::GetNativeStageCreateInfo() const
{
    META_FUNCTION_TASK();
    return vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags{},
        ConvertShaderTypeToStageFlagBits(GetType()),
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

void ShaderVK::InitializeVertexInputDescriptions(const ProgramVK& program)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetType(), Shader::Type::Vertex);
    META_CHECK_ARG_FALSE_DESCR(m_vertex_input_initialized, "vertex input descriptions are already initialized");

    const ProgramBase::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
    m_vertex_input_binding_descriptions.reserve(input_buffer_layouts.size());

    uint32_t input_buffer_index = 0U;
    for(const Program::InputBufferLayout& input_buffer_layout : input_buffer_layouts)
    {
        m_vertex_input_binding_descriptions.emplace_back(
            input_buffer_index++,
            0U, // stride is auto calculated by vertex attributes
            ConvertInputBufferLayoutStepTypeToVertexInputRate(input_buffer_layout.step_type)
        );
    }

    const spirv_cross::Compiler& spirv_compiler = GetNativeCompiler();
    const spirv_cross::ShaderResources shader_resources = spirv_compiler.get_shader_resources();

    m_vertex_input_attribute_descriptions.reserve(shader_resources.stage_inputs.size());
    for(const spirv_cross::Resource& input_resource : shader_resources.stage_inputs)
    {
        const bool has_semantic = spirv_compiler.has_decoration(input_resource.id, spv::DecorationHlslSemanticGOOGLE);
        const bool has_location = spirv_compiler.has_decoration(input_resource.id, spv::DecorationLocation);
        META_CHECK_ARG_TRUE(has_semantic && has_location);

        const std::string&           semantic_name  = spirv_compiler.get_decoration_string(input_resource.id, spv::DecorationHlslSemanticGOOGLE);
        const spirv_cross::SPIRType& attribute_type = spirv_compiler.get_type(input_resource.base_type_id);

        const uint32_t buffer_index = GetProgramInputBufferIndexByArgumentSemantic(program, semantic_name);
        META_CHECK_ARG_LESS(buffer_index, m_vertex_input_binding_descriptions.size());
        vk::VertexInputBindingDescription& input_binding_desc = m_vertex_input_binding_descriptions[buffer_index];

        m_vertex_input_attribute_descriptions.emplace_back(
            spirv_compiler.get_decoration(input_resource.id, spv::DecorationLocation),
            buffer_index,
            GetVertexAttributeFormatFromSpirvType(attribute_type),
            input_binding_desc.stride
        );

        // Tight packing of attributes in vertex buffer is assumed;
        input_binding_desc.stride += attribute_type.vecsize * 4;
    }

    m_vertex_input_initialized = true;
}

const IContextVK& ShaderVK::GetContextVK() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextVK&>(GetContext());
}

} // namespace Methane::Graphics
