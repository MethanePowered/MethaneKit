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
#include "DescriptorManagerDX.h"

#include <Methane/Graphics/CommandListBase.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <directx/d3dx12.h>
#include <magic_enum.hpp>

namespace Methane::Graphics
{

DescriptorsCountByAccess::DescriptorsCountByAccess()
{
    std::fill(m_count_by_access_type.begin(), m_count_by_access_type.end(), 0U);
}

uint32_t& DescriptorsCountByAccess::operator[](IProgram::ArgumentAccessor::Type access_type)
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

uint32_t DescriptorsCountByAccess::operator[](IProgram::ArgumentAccessor::Type access_type) const
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

Ptr<ProgramBindingsBase::ArgumentBindingBase> ProgramBindingsBase::ArgumentBindingBase::CreateCopy(const ArgumentBindingBase& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramBindingsDX::ArgumentBindingDX>(static_cast<const ProgramBindingsDX::ArgumentBindingDX&>(other_argument_binding));
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ContextBase& context, const SettingsDX& settings)
    : ProgramBindingsBase::ArgumentBindingBase(context, settings)
    , m_settings_dx(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME("m_p_descriptor_heap_reservation", !m_p_descriptor_heap_reservation);
}

ProgramBindingsDX::ArgumentBindingDX::ArgumentBindingDX(const ArgumentBindingDX& other)
    : ArgumentBindingBase(other)
    , m_settings_dx(other.m_settings_dx)
    , m_root_parameter_index(other.m_root_parameter_index)
    , m_descriptor_range(other.m_descriptor_range)
    , m_p_descriptor_heap_reservation(other.m_p_descriptor_heap_reservation)
    , m_resource_views_dx(other.m_resource_views_dx)
{
    META_FUNCTION_TASK();
    if (m_p_descriptor_heap_reservation)
    {
        META_CHECK_ARG_TRUE( m_p_descriptor_heap_reservation->heap.get().IsShaderVisible());
        META_CHECK_ARG_EQUAL(m_p_descriptor_heap_reservation->heap.get().GetSettings().type, m_descriptor_range.heap_type);
    }
}

DescriptorHeapDX::Type ProgramBindingsDX::ArgumentBindingDX::GetDescriptorHeapType() const
{
    META_FUNCTION_TASK();
    return (GetSettings().resource_type == Resource::Type::Sampler)
           ? DescriptorHeapDX::Type::Samplers
           : DescriptorHeapDX::Type::ShaderResources;
}

bool ProgramBindingsDX::ArgumentBindingDX::SetResourceViews(const Resource::Views& resource_views)
{
    META_FUNCTION_TASK();
    if (!ArgumentBindingBase::SetResourceViews(resource_views))
        return false;

    if (m_settings_dx.type == Type::DescriptorTable)
    {
        META_CHECK_ARG_LESS_DESCR(resource_views.size(), m_descriptor_range.count + 1, "the number of bound resources exceeds reserved descriptors count");
    }

    const uint32_t             descriptor_range_start = m_p_descriptor_heap_reservation
                                                      ? m_p_descriptor_heap_reservation->GetRange(m_settings_dx.argument.GetAccessorIndex()).GetStart()
                                                      : std::numeric_limits<uint32_t>::max();
    const DescriptorHeapDX*      p_dx_descriptor_heap = m_p_descriptor_heap_reservation
                                                      ? static_cast<const DescriptorHeapDX*>(&m_p_descriptor_heap_reservation->heap.get())
                                                      : nullptr;
    const DescriptorHeapDX::Type descriptor_heap_type = p_dx_descriptor_heap
                                                      ? p_dx_descriptor_heap->GetSettings().type
                                                      : DescriptorHeapDX::Type::Undefined;
    const D3D12_DESCRIPTOR_HEAP_TYPE native_heap_type = p_dx_descriptor_heap
                                                      ? p_dx_descriptor_heap->GetNativeDescriptorHeapType()
                                                      : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    const wrl::ComPtr<ID3D12Device>& cp_native_device = static_cast<const IContextDX&>(GetContext()).GetDeviceDX().GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_native_device);

    uint32_t resource_index = 0;
    m_resource_views_dx.clear();
    m_resource_views_dx.reserve(resource_views.size());
    for(const Resource::View& resource_view : resource_views)
    {
        m_resource_views_dx.emplace_back(resource_view, Resource::Usage::ShaderRead);
        if (!p_dx_descriptor_heap)
            continue;

        const ResourceViewDX& dx_resource_view = m_resource_views_dx.back();
        META_CHECK_ARG_EQUAL_DESCR(m_descriptor_range.heap_type, descriptor_heap_type,
                                   "incompatible heap type '{}' is set for resource binding on argument '{}' of {} shader",
                                   magic_enum::enum_name(descriptor_heap_type), m_settings_dx.argument.GetName(),
                                   magic_enum::enum_name(m_settings_dx.argument.GetShaderType()));

        const uint32_t descriptor_index = descriptor_range_start + m_descriptor_range.offset + resource_index;
        cp_native_device->CopyDescriptorsSimple(
            1,
            p_dx_descriptor_heap->GetNativeCpuDescriptorHandle(descriptor_index),
            dx_resource_view.GetNativeCpuDescriptorHandle(),
            native_heap_type
        );

        resource_index++;
    }

    GetContext().RequestDeferredAction(IContext::DeferredAction::CompleteInitialization);
    return true;
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorRange(const DescriptorRange& descriptor_range)
{
    META_FUNCTION_TASK();
    const DescriptorHeapDX::Type expected_heap_type = GetDescriptorHeapType();
    META_CHECK_ARG_EQUAL_DESCR(descriptor_range.heap_type, expected_heap_type,
                               "descriptor heap type '{}' is incompatible with the resource binding, expected heap type is '{}'",
                               magic_enum::enum_name(descriptor_range.heap_type),
                               magic_enum::enum_name(expected_heap_type));
    META_CHECK_ARG_LESS_DESCR(descriptor_range.count, m_settings_dx.resource_count + 1,
                              "descriptor range size {} will not fit bound shader resources count {}",
                              descriptor_range.count, m_settings_dx.resource_count);

    m_descriptor_range = descriptor_range;
}

void ProgramBindingsDX::ArgumentBindingDX::SetDescriptorHeapReservation(const DescriptorHeapDX::Reservation* p_reservation)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("p_reservation",
                              !p_reservation || (p_reservation->heap.get().IsShaderVisible() && p_reservation->heap.get().GetSettings().type == m_descriptor_range.heap_type),
                              "argument binding reservation must be made in shader visible descriptor heap of type '{}'",
                              magic_enum::enum_name(m_descriptor_range.heap_type));
    m_p_descriptor_heap_reservation = p_reservation;
}

Ptr<ProgramBindings> ProgramBindings::Create(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<ProgramBindingsDX>(program_ptr, resource_views_by_argument, frame_index);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

Ptr<ProgramBindings> ProgramBindings::CreateCopy(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<ProgramBindingsDX>(static_cast<const ProgramBindingsDX&>(other_program_bindings), replace_resource_views_by_argument, frame_index);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

ProgramBindingsDX::ProgramBindingsDX(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
    : ProgramBindingsBase(program_ptr, resource_views_by_argument, frame_index)
{
    META_FUNCTION_TASK();
    ReserveDescriptorHeapRanges();
}

ProgramBindingsDX::ProgramBindingsDX(const ProgramBindingsDX& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
    : ProgramBindingsBase(other_program_bindings, replace_resource_views_by_argument, frame_index)
    , m_descriptor_heap_reservations_by_type(other_program_bindings.m_descriptor_heap_reservations_by_type)
{
    META_FUNCTION_TASK();
    ReserveDescriptorHeapRanges();
}

ProgramBindingsDX::~ProgramBindingsDX()
{
    META_FUNCTION_TASK();
    // Release mutable descriptor ranges in heaps (constant ranges are released by the program)
    for (auto& heap_reservation_opt : m_descriptor_heap_reservations_by_type)
    {
        if (!heap_reservation_opt)
            continue;

        if (const DescriptorHeapDX::Range& mutable_descriptor_range = heap_reservation_opt->ranges[magic_enum::enum_index(IProgram::ArgumentAccessor::Type::Mutable).value()];
            !mutable_descriptor_range.IsEmpty())
        {
            heap_reservation_opt->heap.get().ReleaseRange(mutable_descriptor_range);
        }

        heap_reservation_opt.reset();
    }
}

void ProgramBindingsDX::Initialize()
{
    META_FUNCTION_TASK();
    const ContextBase& context = static_cast<ProgramBase&>(GetProgram()).GetContext();
    DescriptorManagerDX& descriptor_manager = context.GetDescriptorManagerDX();

    descriptor_manager.AddProgramBindings(*this);

    if (descriptor_manager.IsDeferredHeapAllocation())
    {
        context.RequestDeferredAction(IContext::DeferredAction::CompleteInitialization);
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

void ProgramBindingsDX::Apply(CommandListBase& command_list, ApplyBehavior apply_behavior) const
{
    Apply(dynamic_cast<ICommandListDX&>(command_list), command_list.GetProgramBindingsPtr(), apply_behavior);
}

void ProgramBindingsDX::Apply(ICommandListDX& command_list_dx, const ProgramBindingsBase* applied_program_bindings_ptr, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    IProgram::ArgumentAccessor::Type apply_access_mask = IProgram::ArgumentAccessor::Type::Mutable;
    if (!static_cast<bool>(apply_behavior & ApplyBehavior::ConstantOnce) || !applied_program_bindings_ptr)
    {
        apply_access_mask |= IProgram::ArgumentAccessor::Type::Constant;
        apply_access_mask |= IProgram::ArgumentAccessor::Type::FrameConstant;
    }

    // Set resource transition barriers before applying resource bindings
    if (static_cast<bool>(apply_behavior & ApplyBehavior::StateBarriers))
    {
        ApplyResourceTransitionBarriers(command_list_dx, apply_access_mask);
    }

    // Apply root parameter bindings after resource barriers
    ID3D12GraphicsCommandList& d3d12_command_list = command_list_dx.GetNativeCommandList();
    ApplyRootParameterBindings(apply_access_mask, d3d12_command_list, applied_program_bindings_ptr,
                               static_cast<bool>(apply_behavior & ApplyBehavior::ChangesOnly));
}

template<typename FuncType>
void ProgramBindingsDX::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        auto& argument_binding = static_cast<ArgumentBindingDX&>(*argument_binding_ptr);
        const ArgumentBindingDX::DescriptorRange& descriptor_range = argument_binding.GetDescriptorRange();

        if (descriptor_range.heap_type == DescriptorHeapDX::Type::Undefined)
        {
            argument_binding_function(argument_binding, nullptr);
            continue;
        }

        if (const auto& desc_heap_reservation_opt = m_descriptor_heap_reservations_by_type[magic_enum::enum_integer(descriptor_range.heap_type)];
            desc_heap_reservation_opt.has_value())
        {
            argument_binding_function(argument_binding, &*desc_heap_reservation_opt);
            continue;
        }

        argument_binding_function(argument_binding, nullptr);
    }
}

void ProgramBindingsDX::ReserveDescriptorHeapRanges()
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<const ProgramDX&>(GetProgram());
    const uint32_t frames_count = program.GetContext().GetType() == IContext::Type::Render
                                ? dynamic_cast<const RenderContextBase&>(program.GetContext()).GetSettings().frame_buffers_count
                                : 1U;

    // Count the number of constant and mutable descriptors to be allocated in each descriptor heap
    std::map<DescriptorHeapDX::Type, DescriptorsCountByAccess> descriptors_count_by_heap_type;
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        // NOTE: addressable resource bindings do not require descriptors to be created, instead they use direct GPU memory offset from resource
        const auto& binding_settings = argument_binding_ptr->GetSettings();
        if (binding_settings.argument.IsAddressable())
            continue;

        const DescriptorHeapDX::Type           heap_type   = static_cast<const ArgumentBindingDX&>(*argument_binding_ptr).GetDescriptorHeapType();
        const IProgram::ArgumentAccessor::Type access_type = binding_settings.argument.GetAccessorType();

        uint32_t resources_count = binding_settings.resource_count;
        if (access_type == IProgram::ArgumentAccessor::Type::FrameConstant)
        {
            // For Frame Constant bindings we reserve descriptors range for all frames at once
            resources_count *= frames_count;
        }

        descriptors_count_by_heap_type[heap_type][access_type] += resources_count;
    }

    // Reserve descriptor ranges in heaps for resource bindings state
    const DescriptorManagerDX& descriptor_manager = program.GetContext().GetDescriptorManagerDX();
    auto& mutable_program = static_cast<ProgramDX&>(GetProgram());
    for (const auto& [heap_type, descriptors_count] : descriptors_count_by_heap_type)
    {
        std::optional<DescriptorHeapDX::Reservation>& descriptor_heap_reservation_opt = m_descriptor_heap_reservations_by_type[magic_enum::enum_integer(heap_type)];
        if (!descriptor_heap_reservation_opt)
        {
            descriptor_heap_reservation_opt.emplace(descriptor_manager.GetDefaultShaderVisibleDescriptorHeap(heap_type));
        }

        DescriptorHeapDX::Reservation& heap_reservation = *descriptor_heap_reservation_opt;
        META_CHECK_ARG_EQUAL(heap_reservation.heap.get().GetSettings().type, heap_type);
        META_CHECK_ARG_TRUE(heap_reservation.heap.get().GetSettings().shader_visible);

        for (IProgram::ArgumentAccessor::Type access_type : magic_enum::enum_values<IProgram::ArgumentAccessor::Type>())
        {
            const uint32_t accessor_descr_count = descriptors_count[access_type];
            if (!accessor_descr_count)
                continue;

            DescriptorHeapDX::Range& heap_range = heap_reservation.ranges[magic_enum::enum_index(access_type).value()];
            heap_range = mutable_program.ReserveDescriptorRange(heap_reservation.heap.get(), access_type, accessor_descr_count);

            if (access_type == IProgram::ArgumentAccessor::Type::FrameConstant)
            {
                // Since Frame Constant binding range was reserved for all frames at once
                // we need to take only one sub-range related to the frame of current bindings
                const Data::Index frame_range_length = heap_range.GetLength() / frames_count;
                const Data::Index frame_range_start  = heap_range.GetStart() + frame_range_length * GetFrameIndex();
                heap_range = DescriptorHeapDX::Range(frame_range_start, frame_range_start + frame_range_length);
            }
        }
    }
}

void ProgramBindingsDX::AddRootParameterBinding(const IProgram::ArgumentAccessor& argument_accessor, const RootParameterBinding& root_parameter_binding)
{
    META_FUNCTION_TASK();
    m_root_parameter_bindings_by_access[argument_accessor.GetAccessorIndex()].emplace_back(root_parameter_binding);
}

void ProgramBindingsDX::UpdateRootParameterBindings()
{
    META_FUNCTION_TASK();
    for(RootParameterBindings& root_parameter_bindings : m_root_parameter_bindings_by_access)
    {
        root_parameter_bindings.clear();
    }

    ForEachArgumentBinding([this](ArgumentBindingDX& argument_binding, const DescriptorHeapDX::Reservation* p_heap_reservation)
    {
        AddRootParameterBindingsForArgument(argument_binding, p_heap_reservation);
    });
}

void ProgramBindingsDX::AddRootParameterBindingsForArgument(ArgumentBindingDX& argument_binding, const DescriptorHeapDX::Reservation* p_heap_reservation)
{
    META_FUNCTION_TASK();
    using DXBindingType     = ArgumentBindingDX::Type;
    using DXDescriptorRange = ArgumentBindingDX::DescriptorRange;

    const ArgumentBindingDX::SettingsDX& binding_settings = argument_binding.GetSettingsDX();

    if (binding_settings.type == DXBindingType::DescriptorTable)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(p_heap_reservation, "descriptor heap reservation is not available for \"Descriptor Table\" resource binding");
        const auto&              dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
        const DXDescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
        const uint32_t           descriptor_index   = p_heap_reservation->GetRange(binding_settings.argument.GetAccessorIndex()).GetStart() + descriptor_range.offset;

        AddRootParameterBinding(binding_settings.argument, {
            argument_binding,
            argument_binding.GetRootParameterIndex(),
            dx_descriptor_heap.GetNativeGpuDescriptorHandle(descriptor_index),
            0
        });
    }

    for (const ResourceViewDX& resource_view_dx : argument_binding.GetResourceViewsDX())
    {
        if (binding_settings.type == DXBindingType::ConstantBufferView ||
            binding_settings.type == DXBindingType::ShaderResourceView)
        {
            AddRootParameterBinding(binding_settings.argument, {
                argument_binding,
                argument_binding.GetRootParameterIndex(),
                D3D12_GPU_DESCRIPTOR_HANDLE{},
                resource_view_dx.GetNativeGpuAddress()
            });
        }
    }
}

void ProgramBindingsDX::ApplyRootParameterBindings(IProgram::ArgumentAccessor::Type access_types_mask, ID3D12GraphicsCommandList& d3d12_command_list,
                                                   const ProgramBindingsBase* applied_program_bindings_ptr, bool apply_changes_only) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    for(IProgram::ArgumentAccessor::Type access_type : magic_enum::enum_values<IProgram::ArgumentAccessor::Type>())
    {
        if (!static_cast<bool>(access_types_mask & access_type))
            continue;

        const bool do_program_bindings_comparing = access_type == IProgram::ArgumentAccessor::Type::Mutable && apply_changes_only && applied_program_bindings_ptr;
        const RootParameterBindings& root_parameter_bindings = m_root_parameter_bindings_by_access[magic_enum::enum_index(access_type).value()];
        for (const RootParameterBinding& root_parameter_binding : root_parameter_bindings)
        {
            if (do_program_bindings_comparing && root_parameter_binding.argument_binding.IsAlreadyApplied(GetProgram(), *applied_program_bindings_ptr))
                continue;

            ApplyRootParameterBinding(root_parameter_binding, d3d12_command_list);
        }
    }
}

void ProgramBindingsDX::ApplyRootParameterBinding(const RootParameterBinding& root_parameter_binding, ID3D12GraphicsCommandList& d3d12_command_list) const
{
    META_FUNCTION_TASK();
    switch (const ArgumentBindingDX::Type binding_type = root_parameter_binding.argument_binding.GetSettingsDX().type;
            binding_type)
    {
    case ArgumentBindingDX::Type::DescriptorTable:
        d3d12_command_list.SetGraphicsRootDescriptorTable(root_parameter_binding.root_parameter_index, root_parameter_binding.base_descriptor);
        break;

    case ArgumentBindingDX::Type::ConstantBufferView:
        d3d12_command_list.SetGraphicsRootConstantBufferView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    case ArgumentBindingDX::Type::ShaderResourceView:
        d3d12_command_list.SetGraphicsRootShaderResourceView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    default:
        META_UNEXPECTED_ARG(binding_type);
    }
}

void ProgramBindingsDX::CopyDescriptorsToGpu()
{
    META_FUNCTION_TASK();
    META_LOG("Copy descriptors to GPU for program bindings '{}'", GetName());

    const wrl::ComPtr<ID3D12Device>& cp_d3d12_device = static_cast<const ProgramDX&>(GetProgram()).GetContextDX().GetDeviceDX().GetNativeDevice();
    ForEachArgumentBinding([this, &cp_d3d12_device](ArgumentBindingDX& argument_binding, const DescriptorHeapDX::Reservation* p_heap_reservation)
    {
        CopyDescriptorsToGpuForArgument(cp_d3d12_device, argument_binding, p_heap_reservation);
    });
}

void ProgramBindingsDX::CopyDescriptorsToGpuForArgument(const wrl::ComPtr<ID3D12Device>& d3d12_device, ArgumentBindingDX& argument_binding, const DescriptorHeapDX::Reservation* p_heap_reservation) const
{
    META_FUNCTION_TASK();
    if (!p_heap_reservation)
        return;

    using AcceessType = IProgram::ArgumentAccessor::Type;

    const auto&                               dx_descriptor_heap = static_cast<const DescriptorHeapDX&>(p_heap_reservation->heap.get());
    const ArgumentBindingDX::DescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
    const DescriptorHeapDX::Type              heap_type          = dx_descriptor_heap.GetSettings().type;
    const DescriptorHeapDX::Range&            heap_range         = p_heap_reservation->GetRange(argument_binding.GetSettings().argument.GetAccessorIndex());
    const D3D12_DESCRIPTOR_HEAP_TYPE          native_heap_type   = dx_descriptor_heap.GetNativeDescriptorHeapType();

    argument_binding.SetDescriptorHeapReservation(p_heap_reservation);
    META_CHECK_ARG_NOT_NULL(d3d12_device);
    META_CHECK_ARG_LESS_DESCR(descriptor_range.offset, heap_range.GetLength(),
                              "descriptor range offset is out of reserved descriptor range bounds");

    uint32_t resource_index = 0;
    for (const ResourceViewDX& resource_view_dx : argument_binding.GetResourceViewsDX())
    {
        if (!resource_view_dx.HasDescriptor())
            continue;

        META_CHECK_ARG_EQUAL_DESCR(heap_type, resource_view_dx.GetDescriptor()->heap.GetSettings().type,
                                   "can not create binding for resource on descriptor heap of incompatible type");

        const uint32_t descriptor_index = heap_range.GetStart() + descriptor_range.offset + resource_index;
        META_LOG("  - Resource '{}' binding with {} access has descriptor heap range [{}, {}), CPU descriptor index {}",
                 resource_view_dx.GetResourceDX().GetName(),
                 magic_enum::enum_name(argument_binding.GetSettings().argument.GetAccessorType()),
                 descriptor_range.offset, descriptor_range.offset + descriptor_range.count, descriptor_index);

        d3d12_device->CopyDescriptorsSimple(1,
            dx_descriptor_heap.GetNativeCpuDescriptorHandle(descriptor_index),
            resource_view_dx.GetNativeCpuDescriptorHandle(),
            native_heap_type
        );
        resource_index++;
    }
}

} // namespace Methane::Graphics
