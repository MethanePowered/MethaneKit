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

FILE: Methane/Graphics/DirectX12/ShaderDX.cpp
DirectX 12 implementation of the shader interface.

******************************************************************************/

#include "ShaderDX.h"
#include "ProgramDX.h"
#include "ResourceDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <d3dx12.h>
#include <D3Dcompiler.h>
#include <nowide/convert.hpp>

#include <Methane/Data/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>
#include <Methane/Data/Provider.h>

#include <sstream>
#include <cassert>

namespace Methane::Graphics
{

static Resource::Type GetResourceTypeByInputType(D3D_SHADER_INPUT_TYPE input_type)
{
    ITT_FUNCTION_TASK();
    switch (input_type)
    {
    case D3D_SIT_CBUFFER:
    case D3D_SIT_TBUFFER:   return Resource::Type::Buffer;
    case D3D_SIT_TEXTURE:   return Resource::Type::Texture;
    case D3D_SIT_SAMPLER:   return Resource::Type::Sampler;
    default:                throw std::invalid_argument("Unable to determine resource type by DX shader input type.");
    }
}

static std::string GetShaderInputTypeName(D3D_SHADER_INPUT_TYPE input_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (input_type)
    {
    case D3D_SIT_CBUFFER:                       return "CBuffer";
    case D3D_SIT_TBUFFER:                       return "TBuffer";
    case D3D_SIT_TEXTURE:                       return "Texture";
    case D3D_SIT_SAMPLER:                       return "Sampler";
    case D3D_SIT_UAV_RWTYPED:                   return "UAV RW";
    case D3D_SIT_STRUCTURED:                    return "Structured";
    case D3D_SIT_UAV_RWSTRUCTURED:              return "UAV RW Structured";
    case D3D_SIT_BYTEADDRESS:                   return "Byte Address";
    case D3D_SIT_UAV_RWBYTEADDRESS:             return "RW Byte Address";
    case D3D_SIT_UAV_APPEND_STRUCTURED:         return "UAV Append Structured";
    case D3D_SIT_UAV_CONSUME_STRUCTURED:        return "UAV Consume Structured";
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: return "UAV RW Structured with Counter";
    default:                                    assert(0);
    }
    return "Unknown";
}

static std::string GetSRVDimensionName(D3D_SRV_DIMENSION srv_dimension) noexcept
{
    ITT_FUNCTION_TASK();

    switch (srv_dimension)
    {
    case D3D_SRV_DIMENSION_UNKNOWN:             return "Unknown";
    case D3D_SRV_DIMENSION_BUFFER:              return "Buffer";
    case D3D_SRV_DIMENSION_TEXTURE1D:           return "Texture 1D";
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:      return "Texture 1D Array";
    case D3D_SRV_DIMENSION_TEXTURE2D:           return "Texture 2D";
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:      return "Texture 2D Array";
    case D3D_SRV_DIMENSION_TEXTURE2DMS:         return "Texture 2D MS";
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:    return "Texture 2D MS Array";
    case D3D_SRV_DIMENSION_TEXTURE3D:           return "Texture 3D";
    case D3D_SRV_DIMENSION_TEXTURECUBE:         return "Texture Cube";
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:    return "Texture Cube Array";
    case D3D_SRV_DIMENSION_BUFFEREX:            return "Buffer EX";
    default:                                    assert(0);
    }
    return "Unknown";
}

static std::string GetReturnTypeName(D3D_RESOURCE_RETURN_TYPE return_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (return_type)
    {
    case D3D_RETURN_TYPE_UNORM:         return "UNorm";
    case D3D_RETURN_TYPE_SNORM:         return "SNorm";
    case D3D_RETURN_TYPE_SINT:          return "SInt";
    case D3D_RETURN_TYPE_UINT:          return "UInt";
    case D3D_RETURN_TYPE_FLOAT:         return "Float";
    case D3D_RETURN_TYPE_MIXED:         return "Mixed";
    case D3D_RETURN_TYPE_DOUBLE:        return "Double";
    case D3D_RETURN_TYPE_CONTINUED:     return "Continued";
    }
    return "Undefined";
}

static std::string GetValueTypeName(D3D_NAME value_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (value_type)
    {
    case D3D_NAME_UNDEFINED:                        return "Undefined";
    case D3D_NAME_POSITION:                         return "Position";
    case D3D_NAME_CLIP_DISTANCE:                    return "Clip Distance";
    case D3D_NAME_CULL_DISTANCE:                    return "Cull Distance";
    case D3D_NAME_RENDER_TARGET_ARRAY_INDEX:        return "RT Array Index";
    case D3D_NAME_VIEWPORT_ARRAY_INDEX:             return "Viewport Array Index";
    case D3D_NAME_VERTEX_ID:                        return "Vertex ID";
    case D3D_NAME_PRIMITIVE_ID:                     return "Primitive ID";
    case D3D_NAME_INSTANCE_ID:                      return "Instance ID";
    case D3D_NAME_IS_FRONT_FACE:                    return "Is Front Face";
    case D3D_NAME_SAMPLE_INDEX:                     return "Sample Index";
    case D3D_NAME_FINAL_QUAD_EDGE_TESSFACTOR:       return "Final Quad Edge Tess Factor";
    case D3D_NAME_FINAL_QUAD_INSIDE_TESSFACTOR:     return "Final Quad Inside Tess Factor";
    case D3D_NAME_FINAL_TRI_EDGE_TESSFACTOR:        return "Final Tri Edge Tess Factor";
    case D3D_NAME_FINAL_TRI_INSIDE_TESSFACTOR:      return "Final Tri Inside Tess Factor";
    case D3D_NAME_FINAL_LINE_DETAIL_TESSFACTOR:     return "Final Line Detail Tess Factor";
    case D3D_NAME_FINAL_LINE_DENSITY_TESSFACTOR:    return "Final Line Density Tess Factor";
    case D3D_NAME_BARYCENTRICS:                     return "Barycentrics";
    case D3D_NAME_TARGET:                           return "Target";
    case D3D_NAME_DEPTH:                            return "Depth";
    case D3D_NAME_COVERAGE:                         return "Coverage";
    case D3D_NAME_DEPTH_GREATER_EQUAL:              return "Depth Greater Equal";
    case D3D_NAME_DEPTH_LESS_EQUAL:                 return "Depth Less Equal";
    case D3D_NAME_STENCIL_REF:                      return "Stencil Ref";
    case D3D_NAME_INNER_COVERAGE:                   return "Inner Coverage";
    default:                                        assert(0);
    }
    return "Unknown";
}

static std::string GetComponentTypeName(D3D_REGISTER_COMPONENT_TYPE component_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (component_type)
    {
    case D3D_REGISTER_COMPONENT_UNKNOWN:    return "Unknown";
    case D3D_REGISTER_COMPONENT_UINT32:     return "UInt32";
    case D3D_REGISTER_COMPONENT_SINT32:     return "SInt32";
    case D3D_REGISTER_COMPONENT_FLOAT32:    return "Float32";
    default:                                assert(0);
    }
    return "Unknown";
}

using StepType = ProgramBase::InputBufferLayout::StepType;
static D3D12_INPUT_CLASSIFICATION GetInputClassificationByLayoutStepType(StepType step_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (step_type)
    {
    case StepType::PerVertex:     return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    case StepType::PerInstance:   return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    default:                      assert(0);
    }
    return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}

Shader::ResourceBinding::Ptr Shader::ResourceBinding::CreateCopy(const ResourceBinding& other_resource_binging)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderDX::ResourceBindingDX>(static_cast<const ShaderDX::ResourceBindingDX&>(other_resource_binging));
}

ShaderDX::ResourceBindingDX::ResourceBindingDX(ContextBase& context, const Settings& settings)
    : ShaderBase::ResourceBindingBase(context, settings.base)
    , m_settings_dx(settings)
{
    ITT_FUNCTION_TASK();
}

ShaderDX::ResourceBindingDX::ResourceBindingDX(const ResourceBindingDX& other)
    : ResourceBindingBase(other)
    , m_settings_dx(other.m_settings_dx)
    , m_root_parameter_index(other.m_root_parameter_index)
    , m_descriptor_range(other.m_descriptor_range)
    , m_p_descriptor_heap_reservation(other.m_p_descriptor_heap_reservation)
    , m_resource_locations_dx(other.m_resource_locations_dx)
{
    ITT_FUNCTION_TASK();
}

void ShaderDX::ResourceBindingDX::SetResourceLocations(const Resource::Locations& resource_locations)
{
    ITT_FUNCTION_TASK();

    ShaderBase::ResourceBindingBase::SetResourceLocations(resource_locations);

    m_resource_locations_dx.clear();

    if (m_settings_dx.type == Type::DescriptorTable &&
        m_resource_locations.size() > m_descriptor_range.count)
    {
        throw std::invalid_argument("The number of bound resources (" + std::to_string(m_resource_locations.size()) +
                                    ") exceeds reserved descriptors count (" + std::to_string(m_descriptor_range.count) + ").");
    }

    uint32_t resource_index = 0;
    m_resource_locations_dx.reserve(m_resource_locations.size());
    for(const Resource::Location& resource_location : m_resource_locations)
    {
        m_resource_locations_dx.emplace_back(resource_location);

        if (!m_p_descriptor_heap_reservation)
            continue;

        const ResourceDX::LocationDX& resource_location_dx = m_resource_locations_dx.back();
        const DescriptorHeapDX& dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(m_p_descriptor_heap_reservation->heap.get());
        if (m_descriptor_range.heap_type != dx_descriptor_heap.GetSettings().type)
        {
            throw std::logic_error("Incompatible heap type \"" + dx_descriptor_heap.GetTypeName() +
                                   "\" is set for resource binding on argument \"" + GetArgumentName() +
                                   "\" of \"" + Shader::GetTypeName(m_settings.shader_type) + "\" shader.");
        }

        const uint32_t descriptor_index = m_p_descriptor_heap_reservation->GetRange(IsConstant()).GetStart() + m_descriptor_range.offset + resource_index;
        GetContextDX().GetDeviceDX().GetNativeDevice()->CopyDescriptorsSimple(
            1,
            dx_descriptor_heap.GetNativeCPUDescriptorHandle(descriptor_index),
            resource_location_dx.GetResourceDX().GetNativeCPUDescriptorHandle(ResourceBase::Usage::ShaderRead),
            dx_descriptor_heap.GetNativeDescriptorHeapType()
        );

        resource_index++;
    }
}

void ShaderDX::ResourceBindingDX::SetDescriptorRange(const DescriptorRange& descriptor_range)
{
    ITT_FUNCTION_TASK();

    const DescriptorHeap::Type expected_heap_type = GetDescriptorHeapType();
    if (descriptor_range.heap_type != expected_heap_type)
    {
        throw std::runtime_error("Descriptor heap type \"" + DescriptorHeap::GetTypeName(descriptor_range.heap_type) +
                                 "\" is incompatible with the resource binding, expected heap type is \"" +
                                 DescriptorHeap::GetTypeName(expected_heap_type) + "\".");
    }
    if (descriptor_range.count < m_settings_dx.base.resource_count)
    {
        throw std::runtime_error("Descriptor range size (" + std::to_string(descriptor_range.count) + 
                                 ") will not fit bound shader resources (" + std::to_string(m_settings_dx.base.resource_count) + ").");
    }
    m_descriptor_range = descriptor_range;
}

ContextDX& ShaderDX::ResourceBindingDX::GetContextDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextDX&>(m_context);
}

Shader::Ptr Shader::Create(Type type, Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderDX>(type, static_cast<ContextBase&>(context), settings);
}

ShaderDX::ShaderDX(Type type, ContextBase& context, const Settings& settings)
    : ShaderBase(type, context, settings)
{
    ITT_FUNCTION_TASK();

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    const UINT shader_compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    const UINT shader_compile_flags = 0;
#endif

    std::vector<D3D_SHADER_MACRO> macro_definitions;
    for (const auto& definition : m_settings.compile_definitions)
    {
        macro_definitions.push_back({ definition.first.c_str(), definition.second.c_str() });
    }
    macro_definitions.push_back({ nullptr, nullptr });

    wrl::ComPtr<ID3DBlob> error_blob;
    if (!m_settings.source_file_path.empty())
    {
        ThrowIfFailed(D3DCompileFromFile(
            nowide::widen(m_settings.source_file_path).c_str(),
            macro_definitions.data(),
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            m_settings.entry_function.function_name.c_str(),
            m_settings.source_compile_target.c_str(),
            shader_compile_flags,
            0,
            &m_cp_byte_code,
            &error_blob
        ), error_blob);
    }
    else
    {
        const std::string compiled_func_name = GetCompiledEntryFunctionName();
        const Data::Chunk compiled_func_data = m_settings.data_provider.GetData(compiled_func_name + ".obj");

        ThrowIfFailed(D3DCreateBlob(compiled_func_data.size, &m_cp_byte_code));
        Data::RawPtr p_cp_byte_code_data = static_cast<Data::RawPtr>(m_cp_byte_code->GetBufferPointer());
        std::copy(compiled_func_data.p_data, compiled_func_data.p_data + compiled_func_data.size, p_cp_byte_code_data);
    }

    ThrowIfFailed(D3DReflect(
        m_cp_byte_code->GetBufferPointer(),
        m_cp_byte_code->GetBufferSize(),
        IID_PPV_ARGS(&m_cp_reflection)
    ));
}

ShaderBase::ResourceBindings ShaderDX::GetResourceBindings(const std::set<std::string>& constant_argument_names, const std::set<std::string>& addressable_argument_names) const
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_reflection);

    ShaderBase::ResourceBindings resource_bindings;

    D3D12_SHADER_DESC shader_desc = { };
    m_cp_reflection->GetDesc(&shader_desc);

#ifdef _DEBUG
    std::stringstream log_ss;
    log_ss << std::endl << GetTypeName() << " shader v." << shader_desc.Version << " created by \"" << shader_desc.Creator << "\" with resource bindings:" << std::endl;
#endif

    for (UINT resource_index = 0; resource_index < shader_desc.BoundResources; ++resource_index)
    {
        D3D12_SHADER_INPUT_BIND_DESC binding_desc = { };
        ThrowIfFailed(m_cp_reflection->GetResourceBindingDesc(resource_index, &binding_desc));

        const std::string argument_name(binding_desc.Name);
        const bool is_constant_binding    = constant_argument_names.find(argument_name) != constant_argument_names.end();
        const bool is_addressable_binding = addressable_argument_names.find(argument_name) != addressable_argument_names.end();
        const ResourceBindingDX::Type dx_binding_type = !is_addressable_binding ? ResourceBindingDX::Type::DescriptorTable
                                                                                : (binding_desc.Type == D3D_SIT_CBUFFER ? ResourceBindingDX::Type::ConstantBufferView
                                                                                                                        : ResourceBindingDX::Type::ShaderResourceView);
        resource_bindings.push_back(std::make_shared<ResourceBindingDX>(
            m_context,
            ResourceBindingDX::Settings
            {
                {
                    m_type,
                    argument_name,
                    GetResourceTypeByInputType(binding_desc.Type),
                    binding_desc.BindCount,
                    is_constant_binding,
                    is_addressable_binding
                },
                dx_binding_type,
                binding_desc.Type,
                binding_desc.BindPoint,
                binding_desc.Space
            }
        ));

#ifdef _DEBUG
        log_ss << "  - Resource \""  << binding_desc.Name
               << "\" binding "      << resource_index
               << ": type="          << GetShaderInputTypeName(binding_desc.Type)
               << ", dimension="     << GetSRVDimensionName(binding_desc.Dimension)
               << ", return_type="   << GetReturnTypeName(binding_desc.ReturnType)
               << ", samples_count=" << binding_desc.NumSamples
               << ", count="         << binding_desc.BindCount
               << ", point="         << binding_desc.BindPoint
               << ", space="         << binding_desc.Space
               << ", flags="         << binding_desc.uFlags
               << ", id="            << binding_desc.uID
               << std::endl;
#endif
    }

#ifdef _DEBUG
    OutputDebugStringA(log_ss.str().c_str());
#endif

    return resource_bindings;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> ShaderDX::GetNativeProgramInputLayout(const ProgramDX& program) const
{
    ITT_FUNCTION_TASK();
    assert(!!m_cp_reflection);

    D3D12_SHADER_DESC shader_desc = { };
    m_cp_reflection->GetDesc(&shader_desc);

#ifdef _DEBUG
    std::stringstream log_ss;
    log_ss << std::endl << GetTypeName() << " shader input parameters:" << std::endl;
#endif

    std::vector<uint32_t> input_buffer_byte_offsets;
    std::vector<D3D12_INPUT_ELEMENT_DESC> dx_input_layout;
    for (UINT param_index = 0; param_index < shader_desc.InputParameters; ++param_index)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc = { };
        m_cp_reflection->GetInputParameterDesc(param_index, &param_desc);

#ifdef _DEBUG
        log_ss  << "  - Parameter "     << param_index
                << ": semantic_name=\"" << param_desc.SemanticName << "\""
                << ", semantic_index="  << param_desc.SemanticIndex
                << ", register="        << param_desc.Register
                << ", value_type=\""    << GetValueTypeName(param_desc.SystemValueType) << "\""
                << ", component_type="  << GetComponentTypeName(param_desc.ComponentType)
                << ", mask=0x0"         << std::hex << param_desc.Mask
                << ", rw_mask=0x0"      << std::hex << param_desc.ReadWriteMask
                << std::endl;
#endif

        const ProgramBase::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
        const uint32_t buffer_index = GetProgramInputBufferIndexByArgumentSemantic(program, param_desc.SemanticName);
        if (buffer_index > input_buffer_layouts.size())
        {
            throw std::invalid_argument("Provided description of program input layout has insufficient buffers count (" + std::to_string(input_buffer_layouts.size()) +
                                        "), while shader requires buffer at index " + std::to_string(buffer_index) + ".");
        }

        const ProgramBase::InputBufferLayout& input_buffer_layout = input_buffer_layouts[buffer_index];

        if (buffer_index <= input_buffer_byte_offsets.size())
            input_buffer_byte_offsets.resize(buffer_index + 1, 0);

        uint32_t& buffer_byte_offset = input_buffer_byte_offsets[buffer_index];

        uint32_t element_byte_size = 0;
        D3D12_INPUT_ELEMENT_DESC element_desc = { };
        element_desc.SemanticName             = param_desc.SemanticName;
        element_desc.SemanticIndex            = param_desc.SemanticIndex;
        element_desc.InputSlot                = buffer_index;
        element_desc.InputSlotClass           = GetInputClassificationByLayoutStepType(input_buffer_layout.step_type);
        element_desc.InstanceDataStepRate     = 0; // FIXME: input_buffer_layout.step_rate;
        element_desc.Format                   = TypeConverterDX::ParameterDescToDXGIFormatAndSize(param_desc, element_byte_size);
        element_desc.AlignedByteOffset        = buffer_byte_offset;

        dx_input_layout.push_back(element_desc);
        buffer_byte_offset += element_byte_size;
    }

#ifdef _DEBUG
    OutputDebugStringA(log_ss.str().c_str());
#endif

    return dx_input_layout;
}

ContextDX& ShaderDX::GetContextDX()
{
    ITT_FUNCTION_TASK();
    return static_cast<class ContextDX&>(m_context);
}

} // namespace Methane::Graphics
