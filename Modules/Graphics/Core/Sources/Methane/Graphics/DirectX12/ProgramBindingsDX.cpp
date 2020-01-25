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

FILE: Methane/Graphics/DirectX12/ProgramBindingsDX.cpp
DirectX 12 implementation of the program bindings interface.

******************************************************************************/

#include "ProgramBindingsDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "RenderCommandListDX.h"
#include "DescriptorHeapDX.h"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/Windows/Utils.h>

#include <d3dx12.h>
#include <cassert>

namespace Methane::Graphics
{

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
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsDX::ArgumentBindingDX>(static_cast<const ProgramBindingsDX::ArgumentBindingDX&>(other_argument_binding));
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(ContextBase& context, SettingsDX settings)
    : ProgramBindingsBase::ArgumentBindingBase(context, settings)
    , m_settings_dx(std::move(settings))
{
    ITT_FUNCTION_TASK();
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ArgumentBindingDX& other)
    : ArgumentBindingBase(other)
    , m_settings_dx(other.m_settings_dx)
    , m_root_parameter_index(other.m_root_parameter_index)
    , m_descriptor_range(other.m_descriptor_range)
    , m_p_descriptor_heap_reservation(other.m_p_descriptor_heap_reservation)
    , m_resource_locations_dx(other.m_resource_locations_dx)
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsDX::ArgumentBindingDX::SetResourceLocations(const Resource::Locations& resource_locations)
{
    ITT_FUNCTION_TASK();

    ProgramBindingsBase::ArgumentBindingBase::SetResourceLocations(resource_locations);

    m_resource_locations_dx.clear();

    if (m_settings_dx.type == Type::DescriptorTable &&
        resource_locations.size() > m_descriptor_range.count)
    {
        throw std::invalid_argument("The number of bound resources (" + std::to_string(resource_locations.size()) +
                                    ") exceeds reserved descriptors count (" + std::to_string(m_descriptor_range.count) + ").");
    }

    const uint32_t             descriptor_range_start = m_p_descriptor_heap_reservation
                                                      ? m_p_descriptor_heap_reservation->GetRange(m_settings_dx.IsConstant()).GetStart()
                                                      : std::numeric_limits<uint32_t>::max();
    const DescriptorHeapDX*      p_dx_descriptor_heap = m_p_descriptor_heap_reservation
                                                      ? static_cast<const DescriptorHeapDX*>(&m_p_descriptor_heap_reservation->heap.get())
                                                      : nullptr;
    const DescriptorHeap::Type   descriptor_heap_type = p_dx_descriptor_heap
                                                      ? p_dx_descriptor_heap->GetSettings().type
                                                      : DescriptorHeap::Type::Undefined;
    const D3D12_DESCRIPTOR_HEAP_TYPE native_heap_type = p_dx_descriptor_heap
                                                      ? p_dx_descriptor_heap->GetNativeDescriptorHeapType()
                                                      : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    const wrl::ComPtr<ID3D12Device>& cp_native_device = static_cast<class ContextDX&>(GetContext()).GetDeviceDX().GetNativeDevice();
    assert(!!cp_device);

    uint32_t resource_index = 0;
    m_resource_locations_dx.reserve(resource_locations.size());
    for(const Resource::Location& resource_location : resource_locations)
    {
        m_resource_locations_dx.emplace_back(resource_location);

        if (!m_p_descriptor_heap_reservation)
            continue;

        const ResourceDX::LocationDX& dx_resource_location = m_resource_locations_dx.back();
        if (m_descriptor_range.heap_type != descriptor_heap_type)
        {
            throw std::logic_error("Incompatible heap type \"" + p_dx_descriptor_heap->GetTypeName() +
                                   "\" is set for resource binding on argument \"" + m_settings_dx.argument.name +
                                   "\" of \"" + Shader::GetTypeName(m_settings_dx.argument.shader_type) + "\" shader.");
        }

        const uint32_t descriptor_index = descriptor_range_start + m_descriptor_range.offset + resource_index;
        cp_native_device->CopyDescriptorsSimple(
            1,
            p_dx_descriptor_heap->GetNativeCPUDescriptorHandle(descriptor_index),
            dx_resource_location.GetResourceDX().GetNativeCPUDescriptorHandle(ResourceBase::Usage::ShaderRead),
            native_heap_type
        );

        resource_index++;
    }
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorRange(const DescriptorRange& descriptor_range)
{
    ITT_FUNCTION_TASK();

    const DescriptorHeap::Type expected_heap_type = GetDescriptorHeapType();
    if (descriptor_range.heap_type != expected_heap_type)
    {
        throw std::runtime_error("Descriptor heap type \"" + DescriptorHeap::GetTypeName(descriptor_range.heap_type) +
                                 "\" is incompatible with the resource binding, expected heap type is \"" +
                                 DescriptorHeap::GetTypeName(expected_heap_type) + "\".");
    }
    if (descriptor_range.count < m_settings_dx.resource_count)
    {
        throw std::runtime_error("Descriptor range size (" + std::to_string(descriptor_range.count) +
                                 ") will not fit bound shader resources (" + std::to_string(m_settings_dx.resource_count) + ").");
    }
    m_descriptor_range = descriptor_range;
}

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();

    std::shared_ptr<ProgramBindingsDX> sp_dx_program_bindings = std::make_shared<ProgramBindingsDX>(sp_program, resource_locations_by_argument);
    sp_dx_program_bindings->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return sp_dx_program_bindings;
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bingings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();

    std::shared_ptr<ProgramBindingsDX> sp_dx_program_bindings = std::make_shared<ProgramBindingsDX>(static_cast<const ProgramBindingsDX&>(other_program_bingings), replace_resource_locations_by_argument);
    sp_dx_program_bindings->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return sp_dx_program_bindings;
}

ProgramBindingsDX::ProgramBindingsDX(const Ptr<Program>& sp_program, const ResourceLocationsByArgument& resource_locations_by_argument)
    : ProgramBindingsBase(sp_program, resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

ProgramBindingsDX::ProgramBindingsDX(const ProgramBindingsDX& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : ProgramBindingsBase(other_program_bindings, replace_resource_locations_by_argument)
{
    ITT_FUNCTION_TASK();
}

void ProgramBindingsDX::Initialize()
{
    ITT_FUNCTION_TASK();

    ResourceManager& resource_manager = static_cast<ProgramBase&>(*m_sp_program).GetContext().GetResourceManager();
    if (resource_manager.DeferredHeapAllocationEnabled())
    {
        resource_manager.DeferProgramBindingsInitialization(*this);
    }
    else
    {
        CompleteInitialization();
    }
}

void ProgramBindingsDX::CompleteInitialization()
{
    ITT_FUNCTION_TASK();
    CopyDescriptorsToGpu();
}

void ProgramBindingsDX::Apply(CommandList& command_list, ApplyBehavior::Mask apply_behavior) const
{
    ITT_FUNCTION_TASK();

    using DXBindingType = ArgumentBindingDX::Type;
    using DXDescriptorRange = ArgumentBindingDX::DescriptorRange;

    struct GraphicsRootParameterBinding
    {
        DXBindingType               type                 = ArgumentBindingDX::Type::DescriptorTable;
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
    graphics_root_parameter_bindings.reserve(m_binding_by_argument.size());

    ForEachArgumentBinding([&](const Program::Argument& argument, ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        if ((apply_behavior & ApplyBehavior::ConstantOnce || apply_behavior & ApplyBehavior::ChangesOnly) &&
            argument_binding.IsAlreadyApplied(*m_sp_program, argument, command_state, apply_behavior & ApplyBehavior::ChangesOnly))
            return;

        const ArgumentBindingDX::SettingsDX binding_settings = argument_binding.GetSettingsDX();
        const DXBindingType                 binding_type     = binding_settings.type;
        D3D12_GPU_DESCRIPTOR_HANDLE    gpu_descriptor_handle = {};

        if (binding_type == DXBindingType::DescriptorTable)
        {
            if (!p_heap_reservation)
                throw std::runtime_error("Descriptor heap reservation is not available for \"Descriptor Table\" resource binding.");

            const DescriptorHeapDX& dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
            const DXDescriptorRange&  descriptor_range = argument_binding.GetDescriptorRange();
            const uint32_t            descriptor_index = p_heap_reservation->GetRange(binding_settings.IsConstant()).GetStart() + descriptor_range.offset;
            gpu_descriptor_handle = dx_descriptor_heap.GetNativeGPUDescriptorHandle(descriptor_index);
            graphics_root_parameter_bindings.push_back({
                    binding_type,
                    argument_binding.GetRootParameterIndex(),
                    gpu_descriptor_handle,
                    0
                });
        }
        else
        {
            for (const ResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
            {
                graphics_root_parameter_bindings.push_back({
                    binding_type,
                    argument_binding.GetRootParameterIndex(),
                    gpu_descriptor_handle,
                    resource_location_dx.GetNativeGpuAddress()
                });
            }
        }

        if (apply_behavior & ApplyBehavior::StateBarriers)
        {
            const ResourceBase::State resource_state = binding_settings.argument.shader_type == Shader::Type::Pixel
                                                     ? ResourceBase::State::PixelShaderResource
                                                     : ResourceBase::State::NonPixelShaderResource;

            for (const ResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
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

void ProgramBindingsDX::ForEachArgumentBinding(ApplyArgumentBindingFunc apply_argument_binding) const
{
    ITT_FUNCTION_TASK();

    for (auto& binding_by_argument : m_binding_by_argument)
    {
        assert(!!binding_by_argument.second);

        ArgumentBindingDX&                        argument_binding   = static_cast<ArgumentBindingDX&>(*binding_by_argument.second);
        const Program::Argument&                  program_argument   = binding_by_argument.first;
        const ArgumentBindingDX::DescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
        const DescriptorHeap::Reservation*        p_heap_reservation = nullptr;

        if (descriptor_range.heap_type != DescriptorHeap::Type::Undefined)
        {
            const std::optional<DescriptorHeap::Reservation>& descriptor_heap_reservation_opt = m_descriptor_heap_reservations_by_type[static_cast<uint32_t>(descriptor_range.heap_type)];
            if (descriptor_heap_reservation_opt)
            {
                p_heap_reservation = &*descriptor_heap_reservation_opt;
            }
        }

        apply_argument_binding(program_argument, argument_binding, p_heap_reservation);
    }
}

void ProgramBindingsDX::CopyDescriptorsToGpu()
{
    ITT_FUNCTION_TASK();

    assert(!!m_sp_program);
    const wrl::ComPtr<ID3D12Device>& cp_device = static_cast<const ProgramDX&>(*m_sp_program).GetContextDX().GetDeviceDX().GetNativeDevice();
    ForEachArgumentBinding([this, &cp_device](const Program::Argument&, ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        if (!p_heap_reservation)
            return;

        const DescriptorHeapDX&                   dx_descriptor_heap  = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
        const ArgumentBindingDX::DescriptorRange& descriptor_range    = argument_binding.GetDescriptorRange();
        const DescriptorHeap::Type                heap_type           = dx_descriptor_heap.GetSettings().type;
        const bool                                is_constant_bindnig = argument_binding.GetSettings().IsConstant();
        const uint32_t                         descriptor_range_start = p_heap_reservation->GetRange(is_constant_bindnig).GetStart();
        const D3D12_DESCRIPTOR_HEAP_TYPE          native_heap_type    = dx_descriptor_heap.GetNativeDescriptorHeapType();

        argument_binding.SetDescriptorHeapReservation(p_heap_reservation);

        if (descriptor_range.offset >= p_heap_reservation->GetRange(is_constant_bindnig).GetLength())
        {
            throw std::invalid_argument("Descriptor range offset is out of bounds of reserved descriptor range.");
        }

        uint32_t resource_index = 0;
        for (const ResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
        {
            const DescriptorHeap::Types used_heap_types = resource_location_dx.GetResourceDX().GetUsedDescriptorHeapTypes();
            if (used_heap_types.find(heap_type) == used_heap_types.end())
            {
                throw std::invalid_argument("Can not create binding for resource used for " + resource_location_dx.GetResourceDX().GetUsageNames() +
                    " on descriptor heap of incompatible type \"" + dx_descriptor_heap.GetTypeName() + "\".");
            }

            const uint32_t descriptor_index = descriptor_range_start + descriptor_range.offset + resource_index;

            //OutputDebugStringA((dx_resource.GetName() + " range: [" + std::to_string(descriptor_range.offset) + " - " + std::to_string(descriptor_range.offset + descriptor_range.count) + 
            //                    "), descriptor: " + std::to_string(descriptor_index) + "\n").c_str());

            cp_device->CopyDescriptorsSimple(1,
                                             dx_descriptor_heap.GetNativeCPUDescriptorHandle(descriptor_index),
                                             resource_location_dx.GetResourceDX().GetNativeCPUDescriptorHandle(ResourceBase::Usage::ShaderRead),
                                             native_heap_type);
            resource_index++;
        }
    });
}

} // namespace Methane::Graphics
