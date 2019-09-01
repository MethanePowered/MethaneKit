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

FILE: Methane/Graphics/Metal/ShaderMT.mm
Metal implementation of the shader interface.

******************************************************************************/

#include "ShaderMT.hh"
#include "ProgramMT.hh"
#include "ResourceMT.hh"
#include "ContextMT.hh"
#include "DeviceMT.hh"
#include "TypesMT.hh"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane
{
namespace Graphics
{

using StepType = ProgramBase::InputBufferLayout::StepType;
MTLVertexStepFunction GetVertexStepFunction(StepType step_type) noexcept
{
    ITT_FUNCTION_TASK();
    switch(step_type)
    {
        case StepType::Undefined:   return MTLVertexStepFunctionConstant;
        case StepType::PerVertex:   return MTLVertexStepFunctionPerVertex;
        case StepType::PerInstance: return MTLVertexStepFunctionPerInstance;
    }
}

std::string GetMetalArgumentTypeName(MTLArgumentType mtl_arg_type) noexcept
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

std::string GetMetalArgumentAccessName(MTLArgumentAccess mtl_arg_access) noexcept
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

Shader::ResourceBinding::Ptr Shader::ResourceBinding::CreateCopy(const ResourceBinding& other_resource_binging)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderMT::ResourceBindingMT>(static_cast<const ShaderMT::ResourceBindingMT&>(other_resource_binging));
}

ShaderMT::ResourceBindingMT::ResourceBindingMT(ContextBase& context, const Settings& settings)
    : ResourceBindingBase(context, settings.base)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

void ShaderMT::ResourceBindingMT::SetResource(const Resource::Ptr& sp_resource)
{
    ITT_FUNCTION_TASK();

    assert(sp_resource);
    const Resource::Type resource_type = sp_resource->GetResourceType();
    
    bool resource_type_compatible = false;
    switch(m_settings.argument_type)
    {
        case MTLArgumentTypeBuffer:  resource_type_compatible = (resource_type == Resource::Type::Buffer);  break;
        case MTLArgumentTypeTexture: resource_type_compatible = (resource_type == Resource::Type::Texture); break;
        case MTLArgumentTypeSampler: resource_type_compatible = (resource_type == Resource::Type::Sampler); break;
        default:                     assert(0);
    }
    
    if (!resource_type_compatible)
    {
        throw std::invalid_argument("Incompatible resource type \"" + Resource::GetTypeName(sp_resource->GetResourceType()) +
                                    "\" is bound to argument \"" + GetArgumentName() +
                                    "\" of type \"" + GetMetalArgumentTypeName(m_settings.argument_type) + "\".");
    }
    
    ShaderBase::ResourceBindingBase::SetResource(sp_resource);
}

DescriptorHeap::Type ShaderMT::ResourceBindingMT::GetDescriptorHeapType() const
{
    ITT_FUNCTION_TASK();
    return (m_settings.argument_type == MTLArgumentTypeSampler) ? DescriptorHeap::Type::Samplers : DescriptorHeap::Type::ShaderResources;
}

Shader::Ptr Shader::Create(Shader::Type shader_type, Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderMT>(shader_type, static_cast<ContextBase&>(context), settings);
}

ShaderMT::LibraryMT::LibraryMT(ContextMT& metal_context)
    : m_mtl_library([metal_context.GetDeviceMT().GetNativeDevice() newDefaultLibrary])
{
    ITT_FUNCTION_TASK();
}

ShaderMT::LibraryMT::~LibraryMT()
{
    ITT_FUNCTION_TASK();

    [m_mtl_library release];
}

ShaderMT::ShaderMT(Shader::Type shader_type, ContextBase& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
    , m_mtl_function([GetLibraryMT().Get() newFunctionWithName: Methane::MacOS::ConvertToNSType<std::string, NSString*>(GetCompiledEntryFunctionName())])
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

ShaderBase::ResourceBindings ShaderMT::GetResourceBindings(const std::set<std::string>& constant_argument_names) const
{
    ITT_FUNCTION_TASK();

    ShaderBase::ResourceBindings resource_bindings;
    if (m_mtl_arguments == nil)
        return resource_bindings;
    
#ifndef NDEBUG
    NSLog(@"%s shader arguments:", GetTypeName().c_str());
#endif

    for(MTLArgument* mtl_arg in m_mtl_arguments)
    {
        if (mtl_arg.active == NO)
            continue;

        const std::string argument_name = Methane::MacOS::ConvertFromNSType<NSString, std::string>(mtl_arg.name);
        if (argument_name.find("vertexBuffer.") == 0)
        {
            // Skip input vertex buffers, since they are set with a separate RenderCommandList call, not through resource bindings
            continue;
        }
        
        const bool is_constant_binding = constant_argument_names.find(argument_name) != constant_argument_names.end();
        resource_bindings.push_back(std::make_shared<ResourceBindingMT>(
            m_context,
            ResourceBindingMT::Settings
            {
                {
                    m_type,
                    argument_name,
                    is_constant_binding,
                },
                mtl_arg.type,
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

    return resource_bindings;
}

MTLVertexDescriptor* ShaderMT::GetNativeVertexDescriptor(const ProgramMT& program) const
{
    ITT_FUNCTION_TASK();

    MTLVertexDescriptor* mtl_vertex_desc = [[MTLVertexDescriptor alloc] init];
    [mtl_vertex_desc reset];
    
    std::vector<uint32_t> input_buffer_byte_offsets;
    for(MTLVertexAttribute* mtl_vertex_attrib in m_mtl_function.vertexAttributes)
    {
        if (mtl_vertex_attrib.active == NO)
            continue;
        
        const MTLVertexFormat mtl_vertex_format = TypeConverterMT::MetalDataTypeToVertexFormat(mtl_vertex_attrib.attributeType);
        const std::string attrib_name = Methane::MacOS::ConvertFromNSType<NSString, std::string>(mtl_vertex_attrib.name);
        const uint32_t    attrib_size = TypeConverterMT::ByteSizeOfVertexFormat(mtl_vertex_format);
        const uint32_t    attrib_slot = GetProgramInputBufferIndexByArgumentName(program, attrib_name);
        
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

ContextMT& ShaderMT::GetContextMT() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextMT&>(m_context);
}

ShaderMT::LibraryMT& ShaderMT::GetLibraryMT() noexcept
{
    ITT_FUNCTION_TASK();
    static LibraryMT metal_library(GetContextMT());
    return metal_library;
}

} // namespace Graphics
} // namespace Methane
