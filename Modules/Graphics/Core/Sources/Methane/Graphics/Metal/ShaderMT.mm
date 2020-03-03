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

FILE: Methane/Graphics/Metal/ShaderMT.mm
Metal implementation of the shader interface.

******************************************************************************/

#include "ShaderMT.hh"
#include "ProgramMT.hh"
#include "ProgramLibraryMT.hh"
#include "ProgramBindingsMT.hh"
#include "ContextMT.h"
#include "TypesMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

#include <regex>

namespace Methane::Graphics
{

using StepType = ProgramBase::InputBufferLayout::StepType;
static MTLVertexStepFunction GetVertexStepFunction(StepType step_type) noexcept
{
    ITT_FUNCTION_TASK();
    switch(step_type)
    {
        case StepType::Undefined:   return MTLVertexStepFunctionConstant;
        case StepType::PerVertex:   return MTLVertexStepFunctionPerVertex;
        case StepType::PerInstance: return MTLVertexStepFunctionPerInstance;
    }
}

static Resource::Type GetResourceTypeByMetalArgumentType(MTLArgumentType mtl_arg_type)
{
    ITT_FUNCTION_TASK();
    switch(mtl_arg_type)
    {
    case MTLArgumentTypeBuffer:     return Resource::Type::Buffer;
    case MTLArgumentTypeTexture:    return Resource::Type::Texture;
    case MTLArgumentTypeSampler:    return Resource::Type::Sampler;
    default:                        throw std::invalid_argument("Unable to determine resource type by DX shader input type.");
    }
}

#ifndef NDEBUG

static std::string GetMetalArgumentTypeName(MTLArgumentType mtl_arg_type) noexcept
{
    ITT_FUNCTION_TASK();
    switch(mtl_arg_type)
    {
        case MTLArgumentTypeBuffer:             return "Buffer";
        case MTLArgumentTypeThreadgroupMemory:  return "Thread-group Memory";
        case MTLArgumentTypeTexture:            return "Texture";
        case MTLArgumentTypeSampler:            return "Sampler";
        default:                                assert(0);
    }
    return "Unknown";
}

static std::string GetMetalArgumentAccessName(MTLArgumentAccess mtl_arg_access) noexcept
{
    ITT_FUNCTION_TASK();
    switch(mtl_arg_access)
    {
        case MTLArgumentAccessReadOnly:     return "R";
        case MTLArgumentAccessReadWrite:    return "RW";
        case MTLArgumentAccessWriteOnly:    return "W";
        default:                            assert(0);
    }
    return "Unknown";
}
#endif


Ptr<Shader> Shader::Create(Shader::Type shader_type, Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderMT>(shader_type, dynamic_cast<ContextBase&>(context), settings);
}

ShaderMT::ShaderMT(Shader::Type shader_type, ContextBase& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
    , m_mtl_function([context.GetLibraryMT(settings.entry_function.file_name)->Get() newFunctionWithName: Methane::MacOS::ConvertToNsType<std::string, NSString*>(GetCompiledEntryFunctionName())])
{
    ITT_FUNCTION_TASK();

    if (m_mtl_function == nil)
    {
        throw std::runtime_error("Failed to initialize Metal shader function by name '" + GetCompiledEntryFunctionName() + "'");
    }
}

ShaderMT::~ShaderMT()
{
    ITT_FUNCTION_TASK();

    [m_mtl_function release];
}

ShaderBase::ArgumentBindings ShaderMT::GetArgumentBindings(const Program::ArgumentDescriptions& argument_descriptions) const
{
    ITT_FUNCTION_TASK();

    ArgumentBindings argument_bindings;
    if (m_mtl_arguments == nil)
        return argument_bindings;
    
#ifndef NDEBUG
    NSLog(@"%s shader '%s' arguments:", GetTypeName().c_str(), GetCompiledEntryFunctionName().c_str());
#endif

    for(MTLArgument* mtl_arg in m_mtl_arguments)
    {
        if (!mtl_arg.active)
            continue;

        std::string argument_name = Methane::MacOS::ConvertFromNsType<NSString, std::string>(mtl_arg.name);
        if (argument_name.find("vertexBuffer.") == 0)
        {
            // Skip input vertex buffers, since they are set with a separate RenderCommandList call, not through resource bindings
            continue;
        }
        
        const Program::Argument shader_argument(GetType(), argument_name);
        const auto argument_desc_it = Program::FindArgumentDescription(argument_descriptions, shader_argument);
        const Program::ArgumentDesc argument_desc = argument_desc_it == argument_descriptions.end()
                                                  ? Program::ArgumentDesc(shader_argument)
                                                  : *argument_desc_it;
        
        argument_bindings.emplace_back(std::make_shared<ProgramBindingsMT::ArgumentBindingMT>(
            GetContext(),
            ProgramBindingsMT::ArgumentBindingMT::SettingsMT
            {
                {
                    argument_desc,
                    GetResourceTypeByMetalArgumentType(mtl_arg.type),
                    static_cast<uint32_t>(mtl_arg.arrayLength),
                },
                static_cast<uint32_t>(mtl_arg.index)
            }
        ));
        
#ifndef NDEBUG
        const std::string mtl_arg_type_name   = GetMetalArgumentTypeName(mtl_arg.type);
        const std::string mtl_arg_access_name = GetMetalArgumentAccessName(mtl_arg.access);
        if (mtl_arg.arrayLength <= 1)
        {
            NSLog(@"  - %s (%s) with name \"%@\" at index %u", mtl_arg_type_name.c_str(), mtl_arg_access_name.c_str(), mtl_arg.name, (uint32_t) mtl_arg.index);
        }
        else
        {
            NSLog(@"  - %s (%s) array of size %u with name \"%@\" at index %u", mtl_arg_type_name.c_str(), mtl_arg_access_name.c_str(), (uint32_t) mtl_arg.arrayLength, mtl_arg.name, (uint32_t) mtl_arg.index);
        }
#endif
    }

    return argument_bindings;
}

MTLVertexDescriptor* ShaderMT::GetNativeVertexDescriptor(const ProgramMT& program) const
{
    ITT_FUNCTION_TASK();
    
    // Regex matching prefix of the input attributes "in_var_"
    static const std::regex s_attr_suffix_regex("^in_var_");

    MTLVertexDescriptor* mtl_vertex_desc = [[MTLVertexDescriptor alloc] init];
    [mtl_vertex_desc reset];
    
    std::vector<uint32_t> input_buffer_byte_offsets;
    for(MTLVertexAttribute* mtl_vertex_attrib in m_mtl_function.vertexAttributes)
    {
        if (!mtl_vertex_attrib.active)
            continue;
        
        const MTLVertexFormat mtl_vertex_format = TypeConverterMT::MetalDataTypeToVertexFormat(mtl_vertex_attrib.attributeType);
        const std::string attrib_name = std::regex_replace(Methane::MacOS::ConvertFromNsType<NSString, std::string>(mtl_vertex_attrib.name), s_attr_suffix_regex, "");
        const uint32_t    attrib_size = TypeConverterMT::ByteSizeOfVertexFormat(mtl_vertex_format);
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
    
    const ProgramBase::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
    assert(input_buffer_byte_offsets.size() == input_buffer_layouts.size());
    for(uint32_t buffer_index = 0; buffer_index < input_buffer_layouts.size(); ++buffer_index)
    {
        const ProgramBase::InputBufferLayout& input_buffer_layout = input_buffer_layouts[buffer_index];
        MTLVertexBufferLayoutDescriptor* layout_desc = mtl_vertex_desc.layouts[buffer_index];
        layout_desc.stride       = input_buffer_byte_offsets[buffer_index];
        layout_desc.stepRate     = input_buffer_layout.step_rate;
        layout_desc.stepFunction = GetVertexStepFunction(input_buffer_layout.step_type);
    }
    
    return mtl_vertex_desc;
}

IContextMT& ShaderMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextMT&>(GetContext());
}

} // namespace Methane::Graphics
