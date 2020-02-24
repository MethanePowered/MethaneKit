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

FILE: Methane/Graphics/DirectX12/ProgramDX.cpp
DirectX 12 implementation of the program interface.

******************************************************************************/

#include "DeviceDX.h"
#include "ProgramDX.h"
#include "ProgramBindingsDX.h"
#include "ShaderDX.h"
#include "RenderCommandListDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>

#include <d3dx12.h>
#include <D3Dcompiler.h>

#include <nowide/convert.hpp>
#include <cassert>
#include <iomanip>

namespace Methane::Graphics
{

static D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeTypeByShaderInputType(D3D_SHADER_INPUT_TYPE input_type) noexcept
{
    ITT_FUNCTION_TASK();

    switch (input_type)
    {
    case D3D_SIT_CBUFFER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

    case D3D_SIT_SAMPLER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    default:
        assert(0);
    }
    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

static DescriptorHeap::Type GetDescriptorHeapTypeByRangeType(D3D12_DESCRIPTOR_RANGE_TYPE range_type) noexcept
{
    ITT_FUNCTION_TASK();
    if (range_type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
        return DescriptorHeap::Type::Samplers;
    else
        return DescriptorHeap::Type::ShaderResources;
}

static D3D12_SHADER_VISIBILITY GetShaderVisibilityByType(Shader::Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    switch (shader_type)
    {
    case Shader::Type::All:    return D3D12_SHADER_VISIBILITY_ALL;
    case Shader::Type::Vertex: return D3D12_SHADER_VISIBILITY_VERTEX;
    case Shader::Type::Pixel:  return D3D12_SHADER_VISIBILITY_PIXEL;
    default:                   assert(0);
    }
    return D3D12_SHADER_VISIBILITY_ALL;
};

Ptr<Program> Program::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramDX>(dynamic_cast<ContextBase&>(context), settings);
}

ProgramDX::ProgramDX(ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
    , m_dx_input_layout(GetVertexShaderDX().GetNativeProgramInputLayout(*this))
{
    ITT_FUNCTION_TASK();

    InitArgumentBindings(settings.argument_descriptions);
    InitRootSignature();
}

void ProgramDX::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    ObjectBase::SetName(name);

    assert(!!m_cp_root_signature);
    m_cp_root_signature->SetName(nowide::widen(name).c_str());
}

void ProgramDX::InitRootSignature()
{
    ITT_FUNCTION_TASK();

    struct DescriptorOffsets
    {
        uint32_t constant_offset = 0;
        uint32_t mutable_offset = 0;
    };

    using ArgumentBindingDX = ProgramBindingsDX::ArgumentBindingDX;

    std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptor_ranges;
    std::vector<CD3DX12_ROOT_PARAMETER1>   root_parameters;

    const ProgramBindings::ArgumentBindings& binding_by_argument = GetArgumentBindings();
    descriptor_ranges.reserve(binding_by_argument.size());
    root_parameters.reserve(binding_by_argument.size());

    std::map<DescriptorHeap::Type, DescriptorOffsets> descriptor_offset_by_heap_type;
    for (auto& argument_and_binding : binding_by_argument)
    {
        assert(!!argument_and_binding.second);
        const Argument&                    shader_argument = argument_and_binding.first;
        ArgumentBindingDX&                argument_binding = static_cast<ArgumentBindingDX&>(*argument_and_binding.second);
        const ArgumentBindingDX::SettingsDX& bind_settings = argument_binding.GetSettingsDX();
        const D3D12_SHADER_VISIBILITY    shader_visibility = GetShaderVisibilityByType(shader_argument.shader_type);

        argument_binding.SetRootParameterIndex(static_cast<uint32_t>(root_parameters.size()));
        root_parameters.emplace_back();

        switch (bind_settings.type)
        {
        case ArgumentBindingDX::Type::DescriptorTable:
        {
            const D3D12_DESCRIPTOR_RANGE_TYPE  range_type  = GetDescriptorRangeTypeByShaderInputType(bind_settings.input_type);
            const D3D12_DESCRIPTOR_RANGE_FLAGS range_flags = (bind_settings.input_type == D3D_SIT_CBUFFER)
                                                           ? D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
                                                           : D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            
            descriptor_ranges.emplace_back(range_type, bind_settings.resource_count, bind_settings.point, bind_settings.space, range_flags);
            root_parameters.back().InitAsDescriptorTable(1, &descriptor_ranges.back(), shader_visibility);

            const DescriptorHeap::Type  heap_type = GetDescriptorHeapTypeByRangeType(range_type);
            DescriptorOffsets& descriptor_offsets = descriptor_offset_by_heap_type[heap_type];
            uint32_t& descriptor_offset = argument_binding.GetSettings().argument.IsConstant()
                                        ? descriptor_offsets.constant_offset
                                        : descriptor_offsets.mutable_offset;
            argument_binding.SetDescriptorRange({ heap_type, descriptor_offset, bind_settings.resource_count });

            descriptor_offset += bind_settings.resource_count;
        } break;

        case ArgumentBindingDX::Type::ConstantBufferView:
            root_parameters.back().InitAsConstantBufferView(bind_settings.point, bind_settings.space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, shader_visibility);
            break;

        case ArgumentBindingDX::Type::ShaderResourceView:
            root_parameters.back().InitAsShaderResourceView(bind_settings.point, bind_settings.space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, shader_visibility);
            break;
        }
    }

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init_1_1(static_cast<UINT>(root_parameters.size()), root_parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(GetContextDX().GetDeviceDX().GetNativeDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    wrl::ComPtr<ID3DBlob> root_signature_blob;
    wrl::ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_desc, feature_data.HighestVersion, &root_signature_blob, &error_blob), error_blob);
    ThrowIfFailed(GetContextDX().GetDeviceDX().GetNativeDevice()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&m_cp_root_signature)));
}

IContextDX& ProgramDX::GetContextDX() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextDX&>(GetContext());
}

const IContextDX& ProgramDX::GetContextDX() const noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<const IContextDX&>(GetContext());
}

ShaderDX& ProgramDX::GetVertexShaderDX() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<ShaderDX&>(GetShaderRef(Shader::Type::Vertex));
}

ShaderDX& ProgramDX::GetPixelShaderDX() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<ShaderDX&>(GetShaderRef(Shader::Type::Pixel));
}

D3D12_INPUT_LAYOUT_DESC ProgramDX::GetNativeInputLayoutDesc() const noexcept
{
    ITT_FUNCTION_TASK();
    return {
        m_dx_input_layout.data(),
        static_cast<UINT>(m_dx_input_layout.size())
    };
}

} // namespace Methane::Graphics
