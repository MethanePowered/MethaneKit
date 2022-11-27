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

#include <Methane/Graphics/Metal/Shader.hh>
#include <Methane/Graphics/Metal/Program.hh>
#include <Methane/Graphics/Metal/ProgramLibrary.hh>
#include <Methane/Graphics/Metal/ProgramBindings.hh>
#include <Methane/Graphics/Metal/Context.h>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#ifndef NDEBUG
#include <magic_enum.hpp>
#endif

#include <regex>

namespace Methane::Graphics::Rhi
{

Ptr<IShader> IShader::Create(Rhi::ShaderType shader_type, const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::Shader>(shader_type, dynamic_cast<const Base::Context&>(context), settings);
}

} // namespace Methane::Graphics::Rhi

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
static Rhi::IResource::Type GetResourceTypeByMetalArgumentType(MTLArgumentType mtl_arg_type)
{
    META_FUNCTION_TASK();
    switch(mtl_arg_type)
    {
    case MTLArgumentTypeBuffer:     return Rhi::ResourceType::Buffer;
    case MTLArgumentTypeTexture:    return Rhi::ResourceType::Texture;
    case MTLArgumentTypeSampler:    return Rhi::ResourceType::Sampler;
    default:                        META_UNEXPECTED_ARG_RETURN(mtl_arg_type, IResource::Type::Buffer);
    }
}

#ifndef NDEBUG

[[nodiscard]]
static std::string GetMetalArgumentTypeName(MTLArgumentType mtl_arg_type)
{
    META_FUNCTION_TASK();
    switch(mtl_arg_type)
    {
        case MTLArgumentTypeBuffer:             return "Buffer";
        case MTLArgumentTypeThreadgroupMemory:  return "Thread-group Memory";
        case MTLArgumentTypeTexture:            return "Texture";
        case MTLArgumentTypeSampler:            return "Sampler";
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
    , m_mtl_function([dynamic_cast<const IContext&>(context).GetMetalLibrary(settings.entry_function.file_name)->GetNativeLibrary()
                         newFunctionWithName: Methane::MacOS::ConvertToNsType<std::string, NSString*>(GetCompiledEntryFunctionName())])
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(m_mtl_function, "failed to initialize Metal shader function by name '{}'", GetCompiledEntryFunctionName());
}

Ptrs<Base::ProgramArgumentBinding> Shader::GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const
{
    META_FUNCTION_TASK();
    Ptrs<Base::ProgramArgumentBinding> argument_bindings;
    if (m_mtl_arguments == nil)
        return argument_bindings;
    
#ifndef NDEBUG
    NSLog(@"%s shader '%s' arguments:", magic_enum::enum_name(GetType()).data(), GetCompiledEntryFunctionName().c_str());
#endif

    for(MTLArgument* mtl_arg in m_mtl_arguments)
    {
        if (!mtl_arg.active)
            continue;

        std::string argument_name = Methane::MacOS::ConvertFromNsType<NSString, std::string>(mtl_arg.name);
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
        
        argument_bindings.emplace_back(std::make_shared<ProgramBindings::ArgumentBinding>(
            GetContext(),
            ProgramArgumentBindingSettings
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

MTLVertexDescriptor* Shader::GetNativeVertexDescriptor(const Program& program) const
{
    META_FUNCTION_TASK();
    
    // Regex matching prefix of the input attributes "in_var_"
    static const std::regex s_attr_suffix_regex("^in_var_");

    MTLVertexDescriptor* mtl_vertex_desc = [[MTLVertexDescriptor alloc] init];
    [mtl_vertex_desc reset];
    
    std::vector<uint32_t> input_buffer_byte_offsets;
    for(MTLVertexAttribute* mtl_vertex_attrib in m_mtl_function.vertexAttributes)
    {
        if (!mtl_vertex_attrib.active)
            continue;
        
        const MTLVertexFormat mtl_vertex_format = TypeConverter::MetalDataTypeToVertexFormat(mtl_vertex_attrib.attributeType);
        const std::string attrib_name = std::regex_replace(Methane::MacOS::ConvertFromNsType<NSString, std::string>(mtl_vertex_attrib.name), s_attr_suffix_regex, "");
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
