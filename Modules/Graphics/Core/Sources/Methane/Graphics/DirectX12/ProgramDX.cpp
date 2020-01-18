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

FILE: Methane/Graphics/DirectX12/ProgramDX.cpp
DirectX 12 implementation of the program interface.

******************************************************************************/

#include "ContextDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "ShaderDX.h"
#include "ResourceDX.h"
#include "RenderCommandListDX.h"
#include "TypesDX.h"
#include "DescriptorHeapDX.h"

#include <Methane/Data/Instrumentation.h>
#include <Methane/Graphics/Windows/Helpers.h>
#include <Methane/Platform/Windows/Utils.h>

#include <d3dx12.h>
#include <D3Dcompiler.h>

#include <nowide/convert.hpp>
#include <array>
#include <sstream>
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

Ptr<Program::ResourceBindings> Program::ResourceBindings::Create(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();

    std::shared_ptr<ProgramDX::ResourceBindingsDX> sp_dx_resource_bindings = std::make_shared<ProgramDX::ResourceBindingsDX>(sp_program, resource_locations_by_argument);
    sp_dx_resource_bindings->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return sp_dx_resource_bindings;
}

Ptr<Program::ResourceBindings> Program::ResourceBindings::CreateCopy(const ResourceBindings& other_resource_bingings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();

    std::shared_ptr<ProgramDX::ResourceBindingsDX> sp_dx_resource_bindings = std::make_shared<ProgramDX::ResourceBindingsDX>(static_cast<const ProgramDX::ResourceBindingsDX&>(other_resource_bingings), replace_resource_locations_by_argument);
    sp_dx_resource_bindings->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return sp_dx_resource_bindings;
}

ProgramDX::ResourceBindingsDX::ResourceBindingsDX(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
    : ProgramBase::ResourceBindingsBase(sp_program, resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

ProgramDX::ResourceBindingsDX::ResourceBindingsDX(const ResourceBindingsDX& other_resource_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : ProgramBase::ResourceBindingsBase(other_resource_bindings, replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

void ProgramDX::ResourceBindingsDX::Initialize()
{
    ITT_FUNCTION_TASK();

    ResourceManager& resource_manager = static_cast<ProgramBase&>(*m_sp_program).GetContext().GetResourceManager();
    if (resource_manager.DeferredHeapAllocationEnabled())
    {
        resource_manager.DeferResourceBindingsInitialization(*this);
    }
    else
    {
        CompleteInitialization();
    }
}

void ProgramDX::ResourceBindingsDX::CompleteInitialization()
{
    ITT_FUNCTION_TASK();
    CopyDescriptorsToGpu();
}

void ProgramDX::ResourceBindingsDX::Apply(CommandList& command_list, ApplyBehavior::Mask apply_behavior) const
{
    ITT_FUNCTION_TASK();

    using DXBindingType = ShaderDX::ResourceBindingDX::Type;
    using DXDescriptorRange = ShaderDX::ResourceBindingDX::DescriptorRange;

    struct GraphicsRootParameterBinding
    {
        DXBindingType               type                 = ShaderDX::ResourceBindingDX::Type::DescriptorTable;
        uint32_t                    root_parameter_index = 0;
        D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor      = {};
        D3D12_GPU_VIRTUAL_ADDRESS   gpu_virtual_address  = 0u;
    };

    RenderCommandListDX&             render_command_list_dx = dynamic_cast<RenderCommandListDX&>(command_list);
    wrl::ComPtr<ID3D12GraphicsCommandList>& cp_command_list = render_command_list_dx.GetNativeCommandList();
    const CommandListBase::CommandState&    command_state   = render_command_list_dx.GetCommandState();
    assert(!!cp_command_list);

    ResourceBase::Barriers resource_transition_barriers;
    std::vector<GraphicsRootParameterBinding> graphics_root_parameter_bindings;
    graphics_root_parameter_bindings.reserve(m_resource_binding_by_argument.size());

    ForEachResourceBinding([&](const Argument& argument, ShaderDX::ResourceBindingDX& resource_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        if ((apply_behavior & ApplyBehavior::ConstantOnce || apply_behavior & ApplyBehavior::ChangesOnly) &&
            resource_binding.IsAlreadyApplied(*m_sp_program, argument, command_state, apply_behavior & ApplyBehavior::ChangesOnly))
            return;

        const DXBindingType         binding_type            = resource_binding.GetSettings().type;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor_handle   = {};

        if (binding_type == DXBindingType::DescriptorTable)
        {
            if (!p_heap_reservation)
                throw std::runtime_error("Descriptor heap reservation is not available for \"Descriptor Table\" resource binding.");

            const DescriptorHeapDX& dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
            const DXDescriptorRange&  descriptor_range = resource_binding.GetDescriptorRange();
            const uint32_t            descriptor_index = p_heap_reservation->GetRange(resource_binding.IsConstant()).GetStart() + descriptor_range.offset;
            gpu_descriptor_handle = dx_descriptor_heap.GetNativeGPUDescriptorHandle(descriptor_index);
            graphics_root_parameter_bindings.push_back({
                    binding_type,
                    resource_binding.GetRootParameterIndex(),
                    gpu_descriptor_handle,
                    0
                });
        }
        else
        {
            for (const ResourceDX::LocationDX& resource_location_dx : resource_binding.GetResourceLocationsDX())
            {
                graphics_root_parameter_bindings.push_back({
                    binding_type,
                    resource_binding.GetRootParameterIndex(),
                    gpu_descriptor_handle,
                    resource_location_dx.GetNativeGpuAddress()
                });
            }
        }

        if (apply_behavior & ApplyBehavior::StateBarriers)
        {
            const ResourceBase::State resource_state = resource_binding.GetShaderType() == Shader::Type::Pixel
                                                     ? ResourceBase::State::PixelShaderResource
                                                     : ResourceBase::State::NonPixelShaderResource;

            for (const ResourceDX::LocationDX& resource_location_dx : resource_binding.GetResourceLocationsDX())
            {
                resource_location_dx.GetResourceDX().SetState(resource_state, resource_transition_barriers);
            }
        }
    });

    // Set resource transition barriers before applying resource bindings
    if (!resource_transition_barriers.empty())
    {
        render_command_list_dx.SetResourceBarriers(resource_transition_barriers);
    }

    // Apply resource bindings
    for (const GraphicsRootParameterBinding& binding : graphics_root_parameter_bindings)
    {
        switch (binding.type)
        {
        case DXBindingType::DescriptorTable:    cp_command_list->SetGraphicsRootDescriptorTable(binding.root_parameter_index, binding.base_descriptor); break;
        case DXBindingType::ConstantBufferView: cp_command_list->SetGraphicsRootConstantBufferView(binding.root_parameter_index, binding.gpu_virtual_address); break;
        case DXBindingType::ShaderResourceView: cp_command_list->SetComputeRootShaderResourceView(binding.root_parameter_index, binding.gpu_virtual_address); break;
        }
    }
}

void ProgramDX::ResourceBindingsDX::ForEachResourceBinding(ApplyResourceBindingFunc apply_resource_binding) const
{
    ITT_FUNCTION_TASK();

    for (auto& resource_binding_by_argument : m_resource_binding_by_argument)
    {
        assert(!!resource_binding_by_argument.second);

        ShaderDX::ResourceBindingDX&                        resource_binding   = static_cast<ShaderDX::ResourceBindingDX&>(*resource_binding_by_argument.second);
        const Argument&                                     program_argument   = resource_binding_by_argument.first;
        const ShaderDX::ResourceBindingDX::DescriptorRange& descriptor_range   = resource_binding.GetDescriptorRange();
        const DescriptorHeap::Reservation*                  p_heap_reservation = nullptr;

        if (descriptor_range.heap_type != DescriptorHeap::Type::Undefined)
        {
            const std::optional<DescriptorHeap::Reservation>& descriptor_heap_reservation_opt = m_descriptor_heap_reservations_by_type[static_cast<uint32_t>(descriptor_range.heap_type)];
            if (descriptor_heap_reservation_opt)
            {
                p_heap_reservation = &*descriptor_heap_reservation_opt;
            }
        }

        apply_resource_binding(program_argument, resource_binding, p_heap_reservation);
    }
}

void ProgramDX::ResourceBindingsDX::CopyDescriptorsToGpu()
{
    ITT_FUNCTION_TASK();

    assert(!!m_sp_program);
    const wrl::ComPtr<ID3D12Device>& cp_device = static_cast<const ProgramDX&>(*m_sp_program).GetContextDX().GetDeviceDX().GetNativeDevice();
    ForEachResourceBinding([this, &cp_device](const Argument&, ShaderDX::ResourceBindingDX& resource_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        if (!p_heap_reservation)
            return;

        const DescriptorHeapDX&                             dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
        const ShaderDX::ResourceBindingDX::DescriptorRange& descriptor_range   = resource_binding.GetDescriptorRange();
        const DescriptorHeap::Type                          heap_type          = dx_descriptor_heap.GetSettings().type;

        resource_binding.SetDescriptorHeapReservation(p_heap_reservation);

        if (descriptor_range.offset >= p_heap_reservation->GetRange(resource_binding.IsConstant()).GetLength())
        {
            throw std::invalid_argument("Descriptor range offset is out of bounds of reserved descriptor range.");
        }

        uint32_t resource_index = 0;
        for (const ResourceDX::LocationDX& resource_location_dx : resource_binding.GetResourceLocationsDX())
        {
            const DescriptorHeap::Types used_heap_types = resource_location_dx.GetResourceDX().GetUsedDescriptorHeapTypes();
            if (used_heap_types.find(heap_type) == used_heap_types.end())
            {
                throw std::invalid_argument("Can not create binding for resource used for " + resource_location_dx.GetResourceDX().GetUsageNames() +
                    " on descriptor heap of incompatible type \"" + dx_descriptor_heap.GetTypeName() + "\".");
            }

            const uint32_t descriptor_index = p_heap_reservation->GetRange(resource_binding.IsConstant()).GetStart()
                                            + descriptor_range.offset + resource_index;

            //OutputDebugStringA((dx_resource.GetName() + " range: [" + std::to_string(descriptor_range.offset) + " - " + std::to_string(descriptor_range.offset + descriptor_range.count) + 
            //                    "), descriptor: " + std::to_string(descriptor_index) + "\n").c_str());

            cp_device->CopyDescriptorsSimple(1,
                                             dx_descriptor_heap.GetNativeCPUDescriptorHandle(descriptor_index),
                                             resource_location_dx.GetResourceDX().GetNativeCPUDescriptorHandle(ResourceBase::Usage::ShaderRead),
                                             dx_descriptor_heap.GetNativeDescriptorHeapType());
            resource_index++;
        }
    });
}

Ptr<Program> Program::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramDX>(static_cast<ContextBase&>(context), settings);
}

ProgramDX::ProgramDX(ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
    , m_dx_input_layout(GetVertexShaderDX().GetNativeProgramInputLayout(*this))
{
    ITT_FUNCTION_TASK();

    InitResourceBindings(m_settings.constant_argument_names, m_settings.addressable_argument_names);
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

    using ResourceBindingDX = ShaderDX::ResourceBindingDX;

    std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptor_ranges;
    std::vector<CD3DX12_ROOT_PARAMETER1>   root_parameters;

    descriptor_ranges.reserve(m_resource_binding_by_argument.size());
    root_parameters.reserve(m_resource_binding_by_argument.size());

    std::map<DescriptorHeap::Type, DescriptorOffsets> descriptor_offset_by_heap_type;
    for (auto& resource_binding_by_argument : m_resource_binding_by_argument)
    {
        assert(!!resource_binding_by_argument.second);
        const Argument&                  shader_argument = resource_binding_by_argument.first;
        ResourceBindingDX&              resource_binding = static_cast<ShaderDX::ResourceBindingDX&>(*resource_binding_by_argument.second);
        const ResourceBindingDX::Settings& bind_settings = resource_binding.GetSettings();
        const D3D12_SHADER_VISIBILITY  shader_visibility = GetShaderVisibilityByType(shader_argument.shader_type);

        resource_binding.SetRootParameterIndex(static_cast<uint32_t>(root_parameters.size()));
        root_parameters.emplace_back();

        switch (bind_settings.type)
        {
        case ShaderDX::ResourceBindingDX::Type::DescriptorTable:
        {
            const D3D12_DESCRIPTOR_RANGE_TYPE  range_type  = GetDescriptorRangeTypeByShaderInputType(bind_settings.input_type);
            const D3D12_DESCRIPTOR_RANGE_FLAGS range_flags = (bind_settings.input_type == D3D_SIT_CBUFFER)
                                                           ? D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
                                                           : D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            
            descriptor_ranges.emplace_back(range_type, bind_settings.base.resource_count, bind_settings.point, bind_settings.space, range_flags);
            root_parameters.back().InitAsDescriptorTable(1, &descriptor_ranges.back(), shader_visibility);

            const DescriptorHeap::Type  heap_type = GetDescriptorHeapTypeByRangeType(range_type);
            DescriptorOffsets& descriptor_offsets = descriptor_offset_by_heap_type[heap_type];
            uint32_t& descriptor_offset = resource_binding.IsConstant() ? descriptor_offsets.constant_offset : descriptor_offsets.mutable_offset;
            resource_binding.SetDescriptorRange({ heap_type, descriptor_offset, bind_settings.base.resource_count });

            descriptor_offset += bind_settings.base.resource_count;
        } break;

        case ShaderDX::ResourceBindingDX::Type::ConstantBufferView:
            root_parameters.back().InitAsConstantBufferView(bind_settings.point, bind_settings.space, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, shader_visibility);
            break;

        case ShaderDX::ResourceBindingDX::Type::ShaderResourceView:
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

ContextDX& ProgramDX::GetContextDX() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<ContextDX&>(m_context);
}

const ContextDX& ProgramDX::GetContextDX() const noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<const ContextDX&>(m_context);
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
