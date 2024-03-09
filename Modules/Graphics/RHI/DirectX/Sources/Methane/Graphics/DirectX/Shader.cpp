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

FILE: Methane/Graphics/DirectX/Shader.cpp
DirectX 12 implementation of the shader interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/Shader.h>
#include <Methane/Graphics/DirectX/Program.h>
#include <Methane/Graphics/DirectX/ProgramBindings.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Types.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>
#include <Methane/Data/IProvider.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <d3dcompiler.h>

#include <nowide/convert.hpp>
#include <fmt/format.h>
#include <sstream>
#include <set>

#ifdef METHANE_LOGGING_ENABLED
#include <magic_enum.hpp>
#endif

namespace Methane::Graphics::DirectX
{

static const std::set<std::string, std::less<>> g_skip_semantic_names{{ "SV_VERTEXID", "SV_INSTANCEID", "SV_ISFRONTFACE" }};

static Rhi::IResource::Type GetResourceTypeByDimensionType(D3D_SRV_DIMENSION dimension_type)
{
    META_FUNCTION_TASK();
    switch (dimension_type)
    {
    case D3D_SRV_DIMENSION_BUFFER:
    case D3D_SRV_DIMENSION_BUFFEREX:
        return Rhi::IResource::Type::Buffer;

    case D3D_SRV_DIMENSION_TEXTURE1D:
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
    case D3D_SRV_DIMENSION_TEXTURE2D:
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
    case D3D_SRV_DIMENSION_TEXTURE2DMS:
    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
    case D3D_SRV_DIMENSION_TEXTURE3D:
    case D3D_SRV_DIMENSION_TEXTURECUBE:
    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        return Rhi::IResource::Type::Texture;

    default: META_UNEXPECTED_ARG_DESCR_RETURN(dimension_type, Rhi::IResource::Type::Buffer, "unable to determine resource type by DX resource dimension type");
    }
}

[[nodiscard]]
static Rhi::IResource::Type GetResourceTypeByInputAndDimensionType(D3D_SHADER_INPUT_TYPE input_type, D3D_SRV_DIMENSION dimension_type)
{
    META_FUNCTION_TASK();
    switch (input_type)
    {
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_CBUFFER:
    case D3D_SIT_TBUFFER:     return Rhi::IResource::Type::Buffer;
    case D3D_SIT_TEXTURE:     return Rhi::IResource::Type::Texture;
    case D3D_SIT_SAMPLER:     return Rhi::IResource::Type::Sampler;
    case D3D_SIT_UAV_RWTYPED: return GetResourceTypeByDimensionType(dimension_type);
    default: META_UNEXPECTED_ARG_DESCR_RETURN(input_type, Rhi::IResource::Type::Buffer, "unable to determine resource type by DX shader input type");
    }
}

static bool IsUnorderedAccessInputType(D3D_SHADER_INPUT_TYPE input_type) noexcept
{
    return input_type == D3D_SIT_UAV_RWTYPED ||
           input_type == D3D_SIT_UAV_RWSTRUCTURED ||
           input_type == D3D_SIT_UAV_RWBYTEADDRESS ||
           input_type == D3D_SIT_UAV_APPEND_STRUCTURED ||
           input_type == D3D_SIT_UAV_CONSUME_STRUCTURED ||
           input_type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER ||
           input_type == D3D_SIT_UAV_FEEDBACKTEXTURE;
}

using StepType = Base::Program::InputBufferLayout::StepType;

[[nodiscard]]
static D3D12_INPUT_CLASSIFICATION GetInputClassificationByLayoutStepType(StepType step_type)
{
    META_FUNCTION_TASK();
    switch (step_type)
    {
    case StepType::PerVertex:     return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    case StepType::PerInstance:   return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    default:                      META_UNEXPECTED_ARG_RETURN(step_type, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
    }
}

Shader::Shader(Type type, const Base::Context& context, const Settings& settings)
    : Base::Shader(type, context, settings)
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
        m_byte_code_chunk_ptr = std::make_unique<Data::Chunk>(settings.data_provider.GetData(fmt::format("{}.dxil", compiled_func_name)));
    }

    META_CHECK_ARG_NOT_NULL(m_byte_code_chunk_ptr);
    ThrowIfFailed(D3DReflect(m_byte_code_chunk_ptr->GetDataPtr(), m_byte_code_chunk_ptr->GetDataSize(), IID_PPV_ARGS(&m_cp_reflection)));
}

Ptrs<Base::ProgramArgumentBinding> Shader::GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_reflection);

    Ptrs<Base::ProgramArgumentBinding> argument_bindings;

    D3D12_SHADER_DESC shader_desc{};
    m_cp_reflection->GetDesc(&shader_desc);

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << magic_enum::enum_name(GetType()) << " shader v." << shader_desc.Version << " with argument bindings:" << std::endl;
    if (!shader_desc.BoundResources)
        log_ss << "  - No resource bindings.";
#endif

    for (UINT resource_index = 0; resource_index < shader_desc.BoundResources; ++resource_index)
    {
        D3D12_SHADER_INPUT_BIND_DESC binding_desc{};
        ThrowIfFailed(m_cp_reflection->GetResourceBindingDesc(resource_index, &binding_desc));

        const Rhi::ProgramArgument shader_argument(GetType(), Base::Shader::GetCachedArgName(binding_desc.Name));
        const Rhi::ProgramArgumentAccessor* argument_ptr = Rhi::IProgram::FindArgumentAccessor(argument_accessors, shader_argument);
        const Rhi::ProgramArgumentAccessor argument_acc = argument_ptr ? *argument_ptr
                                                        : Rhi::ProgramArgumentAccessor(shader_argument);

        ProgramBindings::ArgumentBinding::Type dx_binding_type = ProgramBindings::ArgumentBinding::Type::DescriptorTable;
        if (argument_acc.IsAddressable())
        {
            if (IsUnorderedAccessInputType(binding_desc.Type))
                // SRV or UAV root descriptors can only be Raw or Structured buffers, textures must be bound through DescriptorTable
                dx_binding_type = binding_desc.Type == D3D_SIT_UAV_RWTYPED
                                ? ProgramBindings::ArgumentBinding::Type::DescriptorTable
                                : ProgramBindings::ArgumentBinding::Type::UnorderedAccessView;
            else
                dx_binding_type = binding_desc.Type == D3D_SIT_CBUFFER
                                ? ProgramBindings::ArgumentBinding::Type::ConstantBufferView
                                : ProgramBindings::ArgumentBinding::Type::ShaderResourceView;
        }

        argument_bindings.push_back(std::make_shared<ProgramBindings::ArgumentBinding>(
            GetContext(),
            ProgramBindings::ArgumentBinding::Settings
            {
                Rhi::IProgramArgumentBinding::Settings
                {
                    argument_acc,
                    GetResourceTypeByInputAndDimensionType(binding_desc.Type, binding_desc.Dimension),
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
               << ": type="         << magic_enum::enum_name(binding_desc.Type)
               << ", dimension="    << magic_enum::enum_name(binding_desc.Dimension)
               << ", return_type="  << magic_enum::enum_name(binding_desc.ReturnType)
               << ", samples_count="<< binding_desc.NumSamples
               << ", count="        << binding_desc.BindCount
               << ", point="        << binding_desc.BindPoint
               << ", space="        << binding_desc.Space
               << ", flags="        << binding_desc.uFlags
               << ", id="           << binding_desc.uID;
        if (argument_acc_it == argument_accessors.end())
        {
            log_ss << ", no user argument description was found, using default";
        }
        if (resource_index < shader_desc.BoundResources - 1)
            log_ss << std::endl;
#endif
    }

    META_LOG(log_ss.str());
    return argument_bindings;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> Shader::GetNativeProgramInputLayout(const Program& program) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_reflection);

    D3D12_SHADER_DESC shader_desc{};
    m_cp_reflection->GetDesc(&shader_desc);

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << magic_enum::enum_name(GetType()) << " shader input parameters:" << std::endl;
    if (!shader_desc.InputParameters)
        log_ss << "  - No input parameters.";
#endif

    std::vector<uint32_t> input_buffer_byte_offsets;
    std::vector<D3D12_INPUT_ELEMENT_DESC> dx_input_layout;
    for (UINT param_index = 0; param_index < shader_desc.InputParameters; ++param_index)
    {
        D3D12_SIGNATURE_PARAMETER_DESC param_desc{};
        m_cp_reflection->GetInputParameterDesc(param_index, &param_desc);

#ifdef METHANE_LOGGING_ENABLED
        log_ss << "  - Parameter "     << param_index
               << ": semantic_name=\"" << param_desc.SemanticName << "\""
               << ", semantic_index="  << param_desc.SemanticIndex
               << ", register="        << param_desc.Register
               << ", value_type="      << magic_enum::enum_name(param_desc.SystemValueType)
               << ", component_type="  << magic_enum::enum_name(param_desc.ComponentType)
               << ", mask=0x0"         << std::hex << param_desc.Mask
               << ", rw_mask=0x0"      << std::hex << param_desc.ReadWriteMask;
        if (param_index < shader_desc.InputParameters - 1)
            log_ss << std::endl;
#endif
        if (g_skip_semantic_names.count(param_desc.SemanticName))
            continue;

        const Base::Program::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
        const uint32_t buffer_index = GetProgramInputBufferIndexByArgumentSemantic(program, param_desc.SemanticName);

        META_CHECK_ARG_LESS_DESCR(buffer_index, input_buffer_layouts.size(),
                                  "Provided description of program input layout has insufficient buffers count {}, while shader requires buffer at index {}",
                                  input_buffer_layouts.size(), buffer_index);
        const Base::Program::InputBufferLayout& input_buffer_layout = input_buffer_layouts[buffer_index];

        if (buffer_index <= input_buffer_byte_offsets.size())
            input_buffer_byte_offsets.resize(buffer_index + 1, 0);

        uint32_t& buffer_byte_offset = input_buffer_byte_offsets[buffer_index];

        uint32_t element_byte_size = 0;
        D3D12_INPUT_ELEMENT_DESC element_desc{};
        element_desc.SemanticName             = param_desc.SemanticName;
        element_desc.SemanticIndex            = param_desc.SemanticIndex;
        element_desc.InputSlot                = buffer_index;
        element_desc.InputSlotClass           = GetInputClassificationByLayoutStepType(input_buffer_layout.step_type);
        element_desc.InstanceDataStepRate     = input_buffer_layout.step_type == StepType::PerVertex ? 0 : input_buffer_layout.step_rate;
        element_desc.Format                   = TypeConverter::ParameterDescToDxgiFormatAndSize(param_desc, element_byte_size);
        element_desc.AlignedByteOffset        = buffer_byte_offset;

        dx_input_layout.push_back(element_desc);
        buffer_byte_offset += element_byte_size;
    }

    META_LOG(log_ss.str().c_str());
    return dx_input_layout;
}

} // namespace Methane::Graphics::DirectX
