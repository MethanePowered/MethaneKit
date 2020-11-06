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

FILE: Methane/Graphics/DirectX12/ProgramBindingsDX.cpp
DirectX 12 implementation of the program bindings interface.

******************************************************************************/

#include "ProgramBindingsDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "CommandListDX.h"
#include "DescriptorHeapDX.h"

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <d3dx12.h>

namespace Methane::Graphics
{

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsDX::ArgumentBindingDX>(static_cast<const ProgramBindingsDX::ArgumentBindingDX&>(other_argument_binding));
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ContextBase& context, const SettingsDX& settings)
    : ProgramBindingsBase::ArgumentBindingBase(context, settings)
    , m_settings_dx(std::move(settings))
{
    META_FUNCTION_TASK();
    META_CHECK_ARG("m_p_descriptor_heap_reservation", !m_p_descriptor_heap_reservation);
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ArgumentBindingDX& other)
    : ArgumentBindingBase(other)
    , m_settings_dx(other.m_settings_dx)
    , m_root_parameter_index(other.m_root_parameter_index)
    , m_descriptor_range(other.m_descriptor_range)
    , m_p_descriptor_heap_reservation(other.m_p_descriptor_heap_reservation)
    , m_resource_locations_dx(other.m_resource_locations_dx)
{
    META_FUNCTION_TASK();
    if (m_p_descriptor_heap_reservation)
    {
        META_CHECK_ARG_TRUE(m_p_descriptor_heap_reservation->heap.get().IsShaderVisible());
        META_CHECK_ARG_EQUAL(m_p_descriptor_heap_reservation->heap.get().GetSettings().type, m_descriptor_range.heap_type);
    }
}

void ProgramBindingsDX::ArgumentBindingDX::SetResourceLocations(const Resource::Locations& resource_locations)
{
    META_FUNCTION_TASK();
    if (GetResourceLocations() == resource_locations)
        return;

    ArgumentBindingBase::SetResourceLocations(resource_locations);

    if (m_settings_dx.type == Type::DescriptorTable)
    {
        META_CHECK_ARG_LESS_DESCR(resource_locations.size(), m_descriptor_range.count + 1, "the number of bound resources exceeds reserved descriptors count");
    }

    const uint32_t             descriptor_range_start = m_p_descriptor_heap_reservation
                                                      ? m_p_descriptor_heap_reservation->GetRange(m_settings_dx.argument.IsConstant()).GetStart()
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
    const wrl::ComPtr<ID3D12Device>& cp_native_device = static_cast<const IContextDX&>(GetContext()).GetDeviceDX().GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_native_device);

    uint32_t resource_index = 0;
    m_resource_locations_dx.clear();
    m_resource_locations_dx.reserve(resource_locations.size());
    for(const Resource::Location& resource_location : resource_locations)
    {
        m_resource_locations_dx.emplace_back(resource_location);

        if (!p_dx_descriptor_heap)
            continue;

        const ResourceDX::LocationDX& dx_resource_location = m_resource_locations_dx.back();
        META_CHECK_ARG_EQUAL_DESCR(m_descriptor_range.heap_type, descriptor_heap_type,
                                   fmt::format("incompatible heap type '{}' is set for resource binding on argument '{}' of {} shader",
                                               DescriptorHeap::GetTypeName(descriptor_heap_type), m_settings_dx.argument.name,
                                               Shader::GetTypeName(m_settings_dx.argument.shader_type)));

        const uint32_t descriptor_index = descriptor_range_start + m_descriptor_range.offset + resource_index;
        cp_native_device->CopyDescriptorsSimple(
            1,
            p_dx_descriptor_heap->GetNativeCpuDescriptorHandle(descriptor_index),
            dx_resource_location.GetResourceDX().GetNativeCpuDescriptorHandle(ResourceBase::Usage::ShaderRead),
            native_heap_type
        );

        resource_index++;
    }

    GetContext().RequestDeferredAction(Context::DeferredAction::CompleteInitialization);
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorRange(const DescriptorRange& descriptor_range)
{
    META_FUNCTION_TASK();
    const DescriptorHeap::Type expected_heap_type = GetDescriptorHeapType();
    META_CHECK_ARG_EQUAL_DESCR(descriptor_range.heap_type, expected_heap_type,
                               fmt::format("descriptor heap type '{}' is incompatible with the resource binding, expected heap type is '{}'",
                                           DescriptorHeap::GetTypeName(descriptor_range.heap_type), DescriptorHeap::GetTypeName(expected_heap_type)));
    META_CHECK_ARG_LESS_DESCR(descriptor_range.count, m_settings_dx.resource_count + 1,
                              fmt::format("descriptor range size {} will not fit bound shader resources count {}", descriptor_range.count, m_settings_dx.resource_count));
    m_descriptor_range = descriptor_range;
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorHeapReservation(const DescriptorHeap::Reservation* p_reservation)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("p_reservation",
                              !p_reservation || (p_reservation->heap.get().IsShaderVisible() && p_reservation->heap.get().GetSettings().type == m_descriptor_range.heap_type),
                              fmt::format("argument binding reservation must be made in shader visible descriptor heap of type '{}'", DescriptorHeap::GetTypeName(m_descriptor_range.heap_type)));
    m_p_descriptor_heap_reservation = p_reservation;
}

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<ProgramBindingsDX>(program_ptr, resource_locations_by_argument);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<ProgramBindingsDX>(static_cast<const ProgramBindingsDX&>(other_program_bindings), replace_resource_locations_by_argument);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

ProgramBindingsDX::ProgramBindingsDX(const Ptr<Program>& program_ptr, const ResourceLocationsByArgument& resource_locations_by_argument)
    : ProgramBindingsBase(program_ptr, resource_locations_by_argument)
{
    META_FUNCTION_TASK();
}

ProgramBindingsDX::ProgramBindingsDX(const ProgramBindingsDX& other_program_bindings, const ResourceLocationsByArgument& replace_resource_locations_by_argument)
    : ProgramBindingsBase(other_program_bindings, replace_resource_locations_by_argument)
{
    META_FUNCTION_TASK();
}

void ProgramBindingsDX::Initialize()
{
    META_FUNCTION_TASK();
    ContextBase&     context = static_cast<ProgramBase&>(GetProgram()).GetContext();
    ResourceManager& resource_manager = context.GetResourceManager();

    resource_manager.AddProgramBindings(*this);

    if (resource_manager.IsDeferredHeapAllocation())
    {
        context.RequestDeferredAction(Context::DeferredAction::CompleteInitialization);
    }
    else
    {
        CompleteInitialization();
    }
}

void ProgramBindingsDX::CompleteInitialization()
{
    META_FUNCTION_TASK();
    CopyDescriptorsToGpu();
    UpdateRootParameterBindings();
}

void ProgramBindingsDX::Apply(CommandListBase& command_list, ApplyBehavior::Mask apply_behavior) const
{
    Apply(dynamic_cast<ICommandListDX&>(command_list), command_list.GetProgramBindings().get(), apply_behavior);
}

void ProgramBindingsDX::Apply(ICommandListDX& command_list_dx, const ProgramBindingsBase* p_applied_program_bindings, ApplyBehavior::Mask apply_behavior) const
{
    META_FUNCTION_TASK();

    const bool apply_constant_resource_bindings = apply_behavior & ~ApplyBehavior::ConstantOnce || !p_applied_program_bindings;
    ID3D12GraphicsCommandList& d3d12_command_list = command_list_dx.GetNativeCommandList();

    // Set resource transition barriers before applying resource bindings
    if (apply_behavior & ApplyBehavior::StateBarriers && ApplyResourceStates(apply_constant_resource_bindings) &&
        m_resource_transition_barriers_ptr && !m_resource_transition_barriers_ptr->IsEmpty())
    {
        command_list_dx.SetResourceBarriersDX(*m_resource_transition_barriers_ptr);
    }

    // Apply root parameter bindings after resource barriers

    if (apply_constant_resource_bindings)
    {
        for (const RootParameterBinding& root_parameter_binding : m_constant_root_parameter_bindings)
        {
            ApplyRootParameterBinding(root_parameter_binding, d3d12_command_list);
        }
    }

    for(const RootParameterBinding& root_parameter_binding : m_variadic_root_parameter_bindings)
    {
        if (apply_behavior & ApplyBehavior::ChangesOnly && p_applied_program_bindings &&
            root_parameter_binding.argument_binding.IsAlreadyApplied(GetProgram(), *p_applied_program_bindings))
            continue;

        ApplyRootParameterBinding(root_parameter_binding, d3d12_command_list);
    }
}

void ProgramBindingsDX::ForEachArgumentBinding(const ArgumentBindingFunc& argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& binding_by_argument : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(binding_by_argument.second);

        auto& argument_binding = static_cast<ArgumentBindingDX&>(*binding_by_argument.second);
        const ArgumentBindingDX::DescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
        const DescriptorHeap::Reservation*        p_heap_reservation = nullptr;

        if (descriptor_range.heap_type != DescriptorHeap::Type::Undefined)
        {
            const std::optional<DescriptorHeap::Reservation>& descriptor_heap_reservation_opt = GetDescriptorHeapReservationByType(descriptor_range.heap_type);
            if (descriptor_heap_reservation_opt)
            {
                p_heap_reservation = &*descriptor_heap_reservation_opt;
            }
        }

        argument_binding_function(argument_binding, p_heap_reservation);
    }
}

void ProgramBindingsDX::AddRootParameterBinding(const Program::ArgumentDesc& argument_desc, RootParameterBinding root_parameter_binding)
{
    META_FUNCTION_TASK();
    if (argument_desc.IsConstant())
    {
        m_constant_root_parameter_bindings.emplace_back(std::move(root_parameter_binding));
    }
    else
    {
        m_variadic_root_parameter_bindings.emplace_back(std::move(root_parameter_binding));
    }
}

void ProgramBindingsDX::AddResourceState(const Program::ArgumentDesc& argument_desc, ResourceState resource_state)
{
    META_FUNCTION_TASK();
    if (argument_desc.IsConstant())
    {
        m_constant_resource_states.emplace_back(std::move(resource_state));
    }
    else
    {
        m_variadic_resource_states.emplace_back(std::move(resource_state));
    }
}

void ProgramBindingsDX::UpdateRootParameterBindings()
{
    META_FUNCTION_TASK();
    using DXBindingType     = ArgumentBindingDX::Type;
    using DXDescriptorRange = ArgumentBindingDX::DescriptorRange;

    m_constant_root_parameter_bindings.clear();
    m_variadic_root_parameter_bindings.clear();
    m_constant_resource_states.clear();
    m_variadic_resource_states.clear();

    ForEachArgumentBinding([this](ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        const ArgumentBindingDX::SettingsDX binding_settings = argument_binding.GetSettingsDX();

        if (binding_settings.type == DXBindingType::DescriptorTable)
        {
            META_CHECK_ARG_NOT_NULL_DESCR(p_heap_reservation, "descriptor heap reservation is not available for \"Descriptor Table\" resource binding");
            const auto&              dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
            const DXDescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
            const uint32_t           descriptor_index   = p_heap_reservation->GetRange(binding_settings.argument.IsConstant()).GetStart() + descriptor_range.offset;

            AddRootParameterBinding(binding_settings.argument, {
                argument_binding,
                argument_binding.GetRootParameterIndex(),
                dx_descriptor_heap.GetNativeGpuDescriptorHandle(descriptor_index),
                0
            });
        }

        for (const ResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
        {
            if (binding_settings.type == DXBindingType::ConstantBufferView ||
                binding_settings.type == DXBindingType::ShaderResourceView)
            {
                AddRootParameterBinding(binding_settings.argument, {
                    argument_binding,
                    argument_binding.GetRootParameterIndex(),
                    D3D12_GPU_DESCRIPTOR_HANDLE{},
                    resource_location_dx.GetNativeGpuAddress()
                });
            }

            const ResourceBase::State non_pixel_resource_state = binding_settings.argument.shader_type == Shader::Type::Vertex &&
                                                                 binding_settings.resource_type == Resource::Type::Buffer
                                                               ? ResourceBase::State::VertexAndConstantBuffer
                                                               : ResourceBase::State::NonPixelShaderResource;
            const ResourceBase::State resource_state = binding_settings.argument.shader_type == Shader::Type::Pixel
                                                     ? ResourceBase::State::PixelShaderResource
                                                     : non_pixel_resource_state;
            AddResourceState(binding_settings.argument, {
                std::dynamic_pointer_cast<ResourceBase>(resource_location_dx.GetResourcePtr()),
                resource_state
            });
        }
    });
}

bool ProgramBindingsDX::ApplyResourceStates(bool apply_constant_resource_states) const
{
    META_FUNCTION_TASK();
    bool resource_states_changed = false;

    if (apply_constant_resource_states)
    {
        for(const ResourceState& resource_state : m_constant_resource_states)
        {
            META_CHECK_ARG_NOT_NULL(resource_state.resource_ptr);
            resource_states_changed |= resource_state.resource_ptr->SetState(resource_state.state, m_resource_transition_barriers_ptr);
        }
    }

    for(const ResourceState& resource_state : m_variadic_resource_states)
    {
        META_CHECK_ARG_NOT_NULL(resource_state.resource_ptr);
        resource_states_changed |= resource_state.resource_ptr->SetState(resource_state.state, m_resource_transition_barriers_ptr);
    }

    return resource_states_changed;
}

void ProgramBindingsDX::ApplyRootParameterBinding(const RootParameterBinding& root_parameter_binding, ID3D12GraphicsCommandList& d3d12_command_list) const
{
    META_FUNCTION_TASK();
    const ArgumentBindingDX::Type binding_type = root_parameter_binding.argument_binding.GetSettingsDX().type;

    switch (binding_type)
    {
    case ArgumentBindingDX::Type::DescriptorTable:
        d3d12_command_list.SetGraphicsRootDescriptorTable(root_parameter_binding.root_parameter_index, root_parameter_binding.base_descriptor);
        break;

    case ArgumentBindingDX::Type::ConstantBufferView:
        d3d12_command_list.SetGraphicsRootConstantBufferView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    case ArgumentBindingDX::Type::ShaderResourceView:
        d3d12_command_list.SetComputeRootShaderResourceView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    default:
        META_UNEXPECTED_ENUM_ARG(binding_type);
    }
}

void ProgramBindingsDX::CopyDescriptorsToGpu()
{
    META_FUNCTION_TASK();
    META_LOG(std::string("Copy descriptors to GPU for program \"") + GetProgram().GetName() + "\"");

    const wrl::ComPtr<ID3D12Device>& cp_device = static_cast<const ProgramDX&>(GetProgram()).GetContextDX().GetDeviceDX().GetNativeDevice();
    ForEachArgumentBinding([&cp_device](ArgumentBindingDX& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        if (!p_heap_reservation)
            return;

        const auto&                               dx_descriptor_heap     = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
        const ArgumentBindingDX::DescriptorRange& descriptor_range       = argument_binding.GetDescriptorRange();
        const DescriptorHeap::Type                heap_type              = dx_descriptor_heap.GetSettings().type;
        const bool                                is_constant_bindinig   = argument_binding.GetSettings().argument.IsConstant();
        const uint32_t                            descriptor_range_start = p_heap_reservation->GetRange(is_constant_bindinig).GetStart();
        const D3D12_DESCRIPTOR_HEAP_TYPE          native_heap_type       = dx_descriptor_heap.GetNativeDescriptorHeapType();

        argument_binding.SetDescriptorHeapReservation(p_heap_reservation);
        META_CHECK_ARG_LESS_DESCR(descriptor_range.offset, p_heap_reservation->GetRange(is_constant_bindinig).GetLength(),
                                     "descriptor range offset is out of reserved descriptor range bounds");

        uint32_t resource_index = 0;
        for (const ResourceDX::LocationDX& resource_location_dx : argument_binding.GetResourceLocationsDX())
        {
            const DescriptorHeap::Types used_heap_types = resource_location_dx.GetResourceDX().GetUsedDescriptorHeapTypes();
            META_CHECK_ARG_DESCR(heap_type, used_heap_types.find(heap_type) != used_heap_types.end(),
                                 fmt::format("can not create binding for resource used for {} on descriptor heap of incompatible type '{}'",
                                             resource_location_dx.GetResourceDX().GetUsageNames(), dx_descriptor_heap.GetTypeName()));

            const uint32_t descriptor_index = descriptor_range_start + descriptor_range.offset + resource_index;
            META_LOG(std::string("  - Resource \"") + resource_location_dx.GetResourceDX().GetName() +
                     "\" range: [" + std::to_string(descriptor_range.offset) + " - " + std::to_string(descriptor_range.offset + descriptor_range.count) +
                     "), descriptor: " + std::to_string(descriptor_index));

            cp_device->CopyDescriptorsSimple(1,
                dx_descriptor_heap.GetNativeCpuDescriptorHandle(descriptor_index),
                resource_location_dx.GetResourceDX().GetNativeCpuDescriptorHandle(ResourceBase::Usage::ShaderRead),
                native_heap_type
            );
            resource_index++;
        }
    });
}

} // namespace Methane::Graphics
