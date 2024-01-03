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

FILE: Methane/Graphics/Metal/Shader.mm
Metal implementation of the shader interface.

******************************************************************************/

#include "Metal/Metal.h"
#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/ProgramLibrary.hh>
#include <Methane/Graphics/Metal/ProgramBindings.hh>
#include <Methane/Graphics/Metal/IContext.h>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#ifndef NDEBUG
#include <magic_enum.hpp>
#endif

#include <regex>

namespace Methane::Graphics::Metal
{

using StepType = Base::Program::InputBufferLayout::StepType;

[[nodiscard]]
static MTLVertexStepFunction GetVertexStepFunction(StepType step_type)
{
    META_FUNCTION_TASK();
    switch(step_type)
    {
        case StepType::Undefined:   return MTLVertexStepFunctionConstant;
        case StepType::PerVertex:   return MTLVertexStepFunctionPerVertex;
        case StepType::PerInstance: return MTLVertexStepFunctionPerInstance;
        default:                    META_UNEXPECTED_ARG_RETURN(step_type, MTLVertexStepFunctionPerVertex);
    }
}

[[nodiscard]]
static Rhi::IResource::Type GetResourceTypeByMetalArgumentType(MTLBindingType mtl_arg_type)
{
    META_FUNCTION_TASK();
    switch(mtl_arg_type)
    {
    case MTLBindingTypeBuffer:  return Rhi::ResourceType::Buffer;
    case MTLBindingTypeTexture: return Rhi::ResourceType::Texture;
    case MTLBindingTypeSampler: return Rhi::ResourceType::Sampler;
    default:                    META_UNEXPECTED_ARG_RETURN(mtl_arg_type, IResource::Type::Buffer);
    }
}

[[nodiscard]]
static uint32_t GetBindingArrayLength(id<MTLBinding> mtl_binding)
{
    META_FUNCTION_TASK();
    if (mtl_binding.type != MTLBindingTypeTexture)
        return 1U;

    id<MTLTextureBinding> mtl_texture_binding = static_cast<id<MTLTextureBinding>>(mtl_binding);
    return static_cast<uint32_t>(mtl_texture_binding.arrayLength);
}

[[nodiscard]]
id<MTLFunction> Shader::GetMetalLibraryFunction(const IContext& context, const Rhi::ShaderSettings& settings)
{
    META_FUNCTION_TASK();
    const std::string compiled_entry_function_name = Base::Shader::GetCompiledEntryFunctionName(settings);

#ifdef METAL_LIBRARY_SPLIT_BY_SHADER_ENTRY_FUNCTION
    std::string_view shader_library_name =  compiled_entry_function_name;
#else
    std::string_view shader_library_name = settings.entry_function.file_name;
#endif

    const Ptr<ProgramLibrary>& program_library_ptr = context.GetMetalLibrary(shader_library_name);
    META_CHECK_ARG_NOT_NULL(program_library_ptr);
    id<MTLLibrary> mtl_shader_library = program_library_ptr->GetNativeLibrary();

#ifdef METAL_LIBRARY_SPLIT_BY_SHADER_ENTRY_FUNCTION
    NSString* ns_shader_function_name = mtl_shader_library.functionNames.firstObject;
#else
    NSString* ns_shader_function_name = Methane::MacOS::ConvertToNsString(compiled_entry_function_name);
#endif

    return [mtl_shader_library newFunctionWithName: ns_shader_function_name];
}

[[nodiscard]]
static std::string GetAttributeName(NSString* ns_reflect_attrib_name)
{
    META_FUNCTION_TASK();

    // Regex matching prefix of the input attributes "in_var_..."
    static const std::regex s_attr_prefix_regex("^in_var_");

    // Regex matching suffix of the input attributes "...123"
    static const std::regex s_attr_suffix_regex("\\d+$");

    std::string attrib_name = MacOS::ConvertFromNsString(ns_reflect_attrib_name);
    attrib_name = std::regex_replace(attrib_name, s_attr_prefix_regex, "");
    attrib_name = std::regex_replace(attrib_name, s_attr_suffix_regex, "");

    std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::toupper);
    return attrib_name;
}

#ifndef NDEBUG

[[nodiscard]]
static std::string GetMetalArgumentTypeName(MTLBindingType mtl_arg_type)
{
    META_FUNCTION_TASK();
    switch(mtl_arg_type)
    {
        case MTLBindingTypeBuffer:             return "Buffer";
        case MTLBindingTypeThreadgroupMemory:  return "Thread-group Memory";
        case MTLBindingTypeTexture:            return "Texture";
        case MTLBindingTypeSampler:            return "Sampler";
        default:                                META_UNEXPECTED_ARG_RETURN(mtl_arg_type, "Unknown");
    }
}

[[nodiscard]]
static std::string GetMetalArgumentAccessName(MTLArgumentAccess mtl_arg_access)
{
    META_FUNCTION_TASK();
    switch(mtl_arg_access)
    {
        case MTLArgumentAccessReadOnly:     return "R";
        case MTLArgumentAccessReadWrite:    return "RW";
        case MTLArgumentAccessWriteOnly:    return "W";
        default:                            META_UNEXPECTED_ARG_RETURN(mtl_arg_access, "Unknown");
    }
}

#endif // ifndef NDEBUG

Shader::Shader(Rhi::ShaderType shader_type, const Base::Context& context, const Settings& settings)
    : Base::Shader(shader_type, context, settings)
    , m_mtl_function(GetMetalLibraryFunction(dynamic_cast<const IContext&>(context), settings))
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_mtl_function, "failed to initialize Metal shader function by name '{}'", GetCompiledEntryFunctionName());
}

Ptrs<Base::ProgramArgumentBinding> Shader::GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const
{
    META_FUNCTION_TASK();
    Ptrs<Base::ProgramArgumentBinding> argument_bindings;
    if (m_mtl_bindings == nil)
        return argument_bindings;
    
#ifndef NDEBUG
    NSLog(@"%s shader '%s' arguments:", magic_enum::enum_name(GetType()).data(), GetCompiledEntryFunctionName().c_str());
#endif

    for(id<MTLBinding> mtl_binding in m_mtl_bindings)
    {
        if (!mtl_binding.argument || !mtl_binding.used)
            continue;

        const std::string argument_name = MacOS::ConvertFromNsString(mtl_binding.name);
        if (argument_name.find("vertexBuffer.") == 0)
        {
            // Skip input vertex buffers, since they are set with a separate IRenderCommandList call, not through resource bindings
            continue;
        }
        
        const Rhi::IProgram::Argument shader_argument(GetType(), Base::Shader::GetCachedArgName(argument_name));
        const auto argument_desc_it = Rhi::IProgram::FindArgumentAccessor(argument_accessors, shader_argument);
        const Rhi::ProgramArgumentAccessor argument_desc = argument_desc_it == argument_accessors.end()
                                                         ? Rhi::ProgramArgumentAccessor(shader_argument)
                                                         : *argument_desc_it;

        const uint32_t array_length = GetBindingArrayLength(mtl_binding);
        argument_bindings.emplace_back(std::make_shared<ProgramBindings::ArgumentBinding>(
            GetContext(),
            ProgramArgumentBindingSettings
            {
                {
                    argument_desc,
                    GetResourceTypeByMetalArgumentType(mtl_binding.type),
                    array_length,
                },
                static_cast<uint32_t>(mtl_binding.index)
            }
        ));
        
#ifndef NDEBUG
        const std::string mtl_arg_type_name   = GetMetalArgumentTypeName(mtl_binding.type);
        const std::string mtl_arg_access_name = GetMetalArgumentAccessName(mtl_binding.access);
        if (array_length <= 1)
        {
            NSLog(@"  - %s (%s) with name \"%@\" at index %u", mtl_arg_type_name.c_str(), mtl_arg_access_name.c_str(), mtl_binding.name, (uint32_t) mtl_binding.index);
        }
        else
        {
            NSLog(@"  - %s (%s) array of size %u with name \"%@\" at index %u", mtl_arg_type_name.c_str(), mtl_arg_access_name.c_str(), array_length, mtl_binding.name, (uint32_t) mtl_binding.index);
        }
#endif
    }

    return argument_bindings;
}

MTLVertexDescriptor* Shader::GetNativeVertexDescriptor(const Program& program) const
{
    META_FUNCTION_TASK();
    MTLVertexDescriptor* mtl_vertex_desc = [[MTLVertexDescriptor alloc] init];
    [mtl_vertex_desc reset];
    
    std::vector<uint32_t> input_buffer_byte_offsets;
    for(MTLVertexAttribute* mtl_vertex_attrib in m_mtl_function.vertexAttributes)
    {
        if (!mtl_vertex_attrib.active)
            continue;
        
        const MTLVertexFormat mtl_vertex_format = TypeConverter::MetalDataTypeToVertexFormat(mtl_vertex_attrib.attributeType);
        const std::string attrib_name = GetAttributeName(mtl_vertex_attrib.name);
        const uint32_t    attrib_size = TypeConverter::ByteSizeOfVertexFormat(mtl_vertex_format);
        const uint32_t    attrib_slot = GetProgramInputBufferIndexByArgumentSemantic(program, attrib_name);
        
        if (attrib_slot <= input_buffer_byte_offsets.size())
            input_buffer_byte_offsets.resize(attrib_slot + 1, 0);
        
        uint32_t& attrib_byte_offset = input_buffer_byte_offsets[attrib_slot];
        
        MTLVertexAttributeDescriptor* mtl_vertex_attrib_desc = mtl_vertex_desc.attributes[mtl_vertex_attrib.attributeIndex];
        mtl_vertex_attrib_desc.format       = mtl_vertex_format;
        mtl_vertex_attrib_desc.bufferIndex  = attrib_slot;
        mtl_vertex_attrib_desc.offset       = attrib_byte_offset;
        
        attrib_byte_offset += attrib_size;
    }
    
    const Base::Program::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
    META_CHECK_ARG_EQUAL(input_buffer_byte_offsets.size(), input_buffer_layouts.size());
    for(uint32_t buffer_index = 0; buffer_index < input_buffer_layouts.size(); ++buffer_index)
    {
        const Base::Program::InputBufferLayout& input_buffer_layout = input_buffer_layouts[buffer_index];
        MTLVertexBufferLayoutDescriptor* layout_desc = mtl_vertex_desc.layouts[buffer_index];
        layout_desc.stride       = input_buffer_byte_offsets[buffer_index];
        layout_desc.stepRate     = input_buffer_layout.step_rate;
        layout_desc.stepFunction = GetVertexStepFunction(input_buffer_layout.step_type);
    }
    
    return mtl_vertex_desc;
}

const IContext& Shader::GetMetalContext() const noexcept
{
    META_FUNCTION_TASK();
    return dynamic_cast<const IContext&>(GetContext());
}

} // namespace Methane::Graphics::Metal
