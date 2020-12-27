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

FILE: Methane/Graphics/DirectX12/ShaderDX.cpp
DirectX 12 implementation of the shader interface.

******************************************************************************/

#include "ShaderDX.h"
#include "ProgramDX.h"
#include "ProgramBindingsDX.h"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Data/Provider.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <sstream>

namespace Methane::Graphics
{

[[nodiscard]]
static Resource::Type GetResourceTypeByInputType(D3D_SHADER_INPUT_TYPE input_type)
{
    META_FUNCTION_TASK();
    switch (input_type)
    {
    case D3D_SIT_CBUFFER:
    case D3D_SIT_TBUFFER:   return Resource::Type::Buffer;
    case D3D_SIT_TEXTURE:   return Resource::Type::Texture;
    case D3D_SIT_SAMPLER:   return Resource::Type::Sampler;
    default: META_UNEXPECTED_ENUM_ARG_DESCR_RETURN(input_type, Resource::Type::Buffer, "unable to determine resource type by DX shader input type");
    }
}

[[nodiscard]]
static std::string GetShaderInputTypeName(D3D_SHADER_INPUT_TYPE input_type)
{
    META_FUNCTION_TASK();
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
    default:                                    META_UNEXPECTED_ENUM_ARG_RETURN(input_type, "Unknown");
    }
}

[[nodiscard]]
static std::string GetSRVDimensionName(D3D_SRV_DIMENSION srv_dimension)
{
    META_FUNCTION_TASK();
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
    default:                                    META_UNEXPECTED_ENUM_ARG_RETURN(srv_dimension, "Unknown");
    }
}

[[nodiscard]]
static std::string GetReturnTypeName(D3D_RESOURCE_RETURN_TYPE return_type)
{
    META_FUNCTION_TASK();
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
    default:                            return "Undefined";
    }
}

static std::string GetValueTypeName(D3D_NAME value_type)
{
    META_FUNCTION_TASK();
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
    default:                                        META_UNEXPECTED_ENUM_ARG_RETURN(value_type, "Unknown");
    }
}

static std::string GetComponentTypeName(D3D_REGISTER_COMPONENT_TYPE component_type)
{
    META_FUNCTION_TASK();
    switch (component_type)
    {
    case D3D_REGISTER_COMPONENT_UNKNOWN:    return "Unknown";
    case D3D_REGISTER_COMPONENT_UINT32:     return "UInt32";
    case D3D_REGISTER_COMPONENT_SINT32:     return "SInt32";
    case D3D_REGISTER_COMPONENT_FLOAT32:    return "Float32";
    default:                                META_UNEXPECTED_ENUM_ARG_RETURN(component_type, "Unknown");
    }
}

using StepType = ProgramBase::InputBufferLayout::StepType;
static D3D12_INPUT_CLASSIFICATION GetInputClassificationByLayoutStepType(StepType step_type)
{
    META_FUNCTION_TASK();
    switch (step_type)
    {
    case StepType::PerVertex:     return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    case StepType::PerInstance:   return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    default:                      META_UNEXPECTED_ENUM_ARG_RETURN(step_type, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
    }
}

Ptr<Shader> Shader::Create(Type type, Context& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<ShaderDX>(type, dynamic_cast<ContextBase&>(context), settings);
}

ShaderDX::ShaderDX(Type type, ContextBase& context, const Settings& settings)
    : ShaderBase(type, context, settings)
{
    META_FUNCTION_TASK();

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    const UINT shader_compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    const UINT shader_compile_flags = 0;
#endif

    std::vector<D3D_SHADER_MACRO> macro_definitions;
    for (const auto& definition : settings.compile_definitions)
    {
        macro_definitions.push_back({ definition.name.c_str(), definition.value.c_str() });
    }
    macro_definitions.push_back({ nullptr, nullptr });

    if (!settings.source_file_path.empty())
    {
        wrl::ComPtr<ID3DBlob> error_blob;
        ThrowIfFailed(D3DCompileFromFile(
            nowide::widen(settings.source_file_path).c_str(),
            macro_definitions.data(),
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            settings.entry_function.function_name.c_str(),
            settings.source_compile_target.c_str(),
            shader_compile_flags,
            0,
            &m_cp_byte_code,
            &error_blob
        ), error_blob);

        m_byte_code_chunk_ptr = std::make_unique<Data::Chunk>(static_cast<Data::ConstRawPtr>(m_cp_byte_code->GetBufferPointer()),
                                                             static_cast<Data::Size>(m_cp_byte_code->GetBufferSize()));
    }
    else
    {
        const std::string compiled_func_name = GetCompiledEntryFunctionName();
        m_byte_code_chunk_ptr = std::make_unique<Data::Chunk>(settings.data_provider.GetData(compiled_func_name + ".obj"));
    }

    META_CHECK_ARG_NOT_NULL(m_byte_code_chunk_ptr);
    ThrowIfFailed(D3DReflect(m_byte_code_chunk_ptr->GetDataPtr(), m_byte_code_chunk_ptr->GetDataSize(), IID_PPV_ARGS(&m_cp_reflection)));
}

ShaderBase::ArgumentBindings ShaderDX::GetArgumentBindings(const Program::ArgumentDescriptions& argument_descriptions) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_reflection);

    ShaderBase::ArgumentBindings argument_bindings;

    D3D12_SHADER_DESC shader_desc{};
    m_cp_reflection->GetDesc(&shader_desc);

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << std::endl << magic_enum::flags::enum_name(GetType()) << " shader v." << shader_desc.Version << " with argument bindings:" << std::endl;
#endif

    for (UINT resource_index = 0; resource_index < shader_desc.BoundResources; ++resource_index)
    {
        D3D12_SHADER_INPUT_BIND_DESC binding_desc{};
        ThrowIfFailed(m_cp_reflection->GetResourceBindingDesc(resource_index, &binding_desc));

        const Program::Argument shader_argument(GetType(), binding_desc.Name);
        const auto argument_desc_it = Program::FindArgumentDescription(argument_descriptions, shader_argument);
        const Program::ArgumentDesc argument_desc = argument_desc_it == argument_descriptions.end()
                                                  ? Program::ArgumentDesc(shader_argument)
                                                  : *argument_desc_it;
        const ProgramBindingsDX::ArgumentBindingDX::Type dx_addressable_binding_type = binding_desc.Type == D3D_SIT_CBUFFER
                                                  ? ProgramBindingsDX::ArgumentBindingDX::Type::ConstantBufferView
                                                  : ProgramBindingsDX::ArgumentBindingDX::Type::ShaderResourceView;
        const ProgramBindingsDX::ArgumentBindingDX::Type dx_binding_type = argument_desc.IsAddressable()
                                                  ? dx_addressable_binding_type
                                                  : ProgramBindingsDX::ArgumentBindingDX::Type::DescriptorTable;

        argument_bindings.push_back(std::make_shared<ProgramBindingsDX::ArgumentBindingDX>(
            GetContext(),
            ProgramBindingsDX::ArgumentBindingDX::SettingsDX
            {
                {
                    argument_desc,
                    GetResourceTypeByInputType(binding_desc.Type),
                    binding_desc.BindCount
                },
                dx_binding_type,
                binding_desc.Type,
                binding_desc.BindPoint,
                binding_desc.Space
            }
        ));

#ifdef METHANE_LOGGING_ENABLED
        log_ss << "  - Argument \"" << binding_desc.Name
               << "\" binding "     << resource_index
               << ": type="         << GetShaderInputTypeName(binding_desc.Type)
               << ", dimension="    << GetSRVDimensionName(binding_desc.Dimension)
               << ", return_type="  << GetReturnTypeName(binding_desc.ReturnType)
               << ", samples_count="<< binding_desc.NumSamples
               << ", count="        << binding_desc.BindCount
               << ", point="        << binding_desc.BindPoint
               << ", space="        << binding_desc.Space
               << ", flags="        << binding_desc.uFlags
               << ", id="           << binding_desc.uID;
        if (argument_desc_it == argument_descriptions.end())
        {
            log_ss << ", no user argument description was found, using default";
        }
        log_ss << std::endl;
#endif
    }

    META_LOG(log_ss.str());
    return argument_bindings;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> ShaderDX::GetNativeProgramInputLayout(const ProgramDX& program) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_reflection);

    D3D12_SHADER_DESC shader_desc{};
    m_cp_reflection->GetDesc(&shader_desc);

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << std::endl << magic_enum::flags::enum_name(GetType()) << " shader input parameters:" << std::endl;
#endif

    std::vector<uint32_t> input_buffer_byte_offsets;
    std::vector<D3D12_INPUT_ELEMENT_DESC> dx_input_layout;
    for (UINT param_index = 0; param_index < shader_desc.InputParameters; ++param_index)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc{};
        m_cp_reflection->GetInputParameterDesc(param_index, &param_desc);

#ifdef METHANE_LOGGING_ENABLED
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

        META_CHECK_ARG_LESS_DESCR(buffer_index, input_buffer_layouts.size(),
                                  "Provided description of program input layout has insufficient buffers count {}, while shader requires buffer at index {}",
                                  input_buffer_layouts.size(), buffer_index);
        const ProgramBase::InputBufferLayout& input_buffer_layout = input_buffer_layouts[buffer_index];

        if (buffer_index <= input_buffer_byte_offsets.size())
            input_buffer_byte_offsets.resize(buffer_index + 1, 0);

        uint32_t& buffer_byte_offset = input_buffer_byte_offsets[buffer_index];

        uint32_t element_byte_size = 0;
        D3D12_INPUT_ELEMENT_DESC element_desc{};
        element_desc.SemanticName             = param_desc.SemanticName;
        element_desc.SemanticIndex            = param_desc.SemanticIndex;
        element_desc.InputSlot                = buffer_index;
        element_desc.InputSlotClass           = GetInputClassificationByLayoutStepType(input_buffer_layout.step_type);
        element_desc.InstanceDataStepRate     = 0; // FIXME: use input_buffer_layout.step_rate
        element_desc.Format                   = TypeConverterDX::ParameterDescToDxgiFormatAndSize(param_desc, element_byte_size);
        element_desc.AlignedByteOffset        = buffer_byte_offset;

        dx_input_layout.push_back(element_desc);
        buffer_byte_offset += element_byte_size;
    }

    META_LOG(log_ss.str().c_str());
    return dx_input_layout;
}

IContextDX& ShaderDX::GetContextDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<IContextDX&>(GetContext());
}

} // namespace Methane::Graphics
