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

FILE: Methane/Graphics/DirectX/ProgramBindings.cpp
DirectX 12 implementation of the program bindings interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/ProgramBindings.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Program.h>
#include <Methane/Graphics/DirectX/CommandListSet.h>
#include <Methane/Graphics/DirectX/DescriptorHeap.h>
#include <Methane/Graphics/DirectX/DescriptorManager.h>

#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <directx/d3dx12.h>
#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IProgramBindings> IProgramBindings::Create(const Ptr<IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<DirectX::ProgramBindings>(program_ptr, resource_views_by_argument, frame_index);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

Ptr<IProgramBindings> IProgramBindings::CreateCopy(const Rhi::IProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    const auto dx_program_bindings_ptr = std::make_shared<DirectX::ProgramBindings>(static_cast<const DirectX::ProgramBindings&>(other_program_bindings), replace_resource_views_by_argument, frame_index);
    dx_program_bindings_ptr->Initialize(); // NOTE: Initialize is called externally (not from constructor) to enable using shared_from_this from its code
    return dx_program_bindings_ptr;
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

DescriptorsCountByAccess::DescriptorsCountByAccess()
{
    std::fill(m_count_by_access_type.begin(), m_count_by_access_type.end(), 0U);
}

uint32_t& DescriptorsCountByAccess::operator[](Rhi::ProgramArgumentAccessor::Type access_type)
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

uint32_t DescriptorsCountByAccess::operator[](Rhi::ProgramArgumentAccessor::Type access_type) const
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

ProgramBindings::ProgramBindings(const Ptr<Rhi::IProgram>& program_ptr, const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index)
    : Base::ProgramBindings(program_ptr, resource_views_by_argument, frame_index)
{
    META_FUNCTION_TASK();
    ReserveDescriptorHeapRanges();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings, const ResourceViewsByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
    : Base::ProgramBindings(other_program_bindings, replace_resource_views_by_argument, frame_index)
    , m_descriptor_heap_reservations_by_type(other_program_bindings.m_descriptor_heap_reservations_by_type)
{
    META_FUNCTION_TASK();
    ReserveDescriptorHeapRanges();
}

ProgramBindings::~ProgramBindings()
{
    META_FUNCTION_TASK();
    // Release mutable descriptor ranges in heaps (constant ranges are released by the program)
    for (auto& heap_reservation_opt : m_descriptor_heap_reservations_by_type)
    {
        if (!heap_reservation_opt)
            continue;

        if (const DescriptorHeap::Range& mutable_descriptor_range = heap_reservation_opt->ranges[magic_enum::enum_index(Rhi::ProgramArgumentAccessor::Type::Mutable).value()];
            !mutable_descriptor_range.IsEmpty())
        {
            heap_reservation_opt->heap.get().ReleaseRange(mutable_descriptor_range);
        }

        heap_reservation_opt.reset();
    }
}

void ProgramBindings::Initialize()
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<Program&>(GetProgram());
    DescriptorManager& descriptor_manager = program.GetDirectContext().GetDirectDescriptorManager();

    descriptor_manager.AddProgramBindings(*this);

    if (descriptor_manager.IsDeferredHeapAllocation())
    {
        program.GetContext().RequestDeferredAction(Rhi::IContext::DeferredAction::CompleteInitialization);
    }
    else
    {
        CompleteInitialization();
    }
}

void ProgramBindings::CompleteInitialization()
{
    META_FUNCTION_TASK();
    CopyDescriptorsToGpu();
    UpdateRootParameterBindings();
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehavior apply_behavior) const
{
    Apply(dynamic_cast<ICommandListDx&>(command_list), command_list.GetProgramBindingsPtr(), apply_behavior);
}

void ProgramBindings::Apply(ICommandListDx& command_list_dx, const Base::ProgramBindings* applied_program_bindings_ptr, ApplyBehavior apply_behavior) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    Rhi::ProgramArgumentAccessor::Type apply_access_mask = Rhi::ProgramArgumentAccessor::Type::Mutable;
    if (!static_cast<bool>(apply_behavior & ApplyBehavior::ConstantOnce) || !applied_program_bindings_ptr)
    {
        apply_access_mask |= Rhi::ProgramArgumentAccessor::Type::Constant;
        apply_access_mask |= Rhi::ProgramArgumentAccessor::Type::FrameConstant;
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
void ProgramBindings::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL(argument_binding_ptr);
        auto& argument_binding = static_cast<ArgumentBinding&>(*argument_binding_ptr);
        const ArgumentBinding::DescriptorRange& descriptor_range = argument_binding.GetDescriptorRange();

        if (descriptor_range.heap_type == DescriptorHeap::Type::Undefined)
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

void ProgramBindings::ReserveDescriptorHeapRanges()
{
    META_FUNCTION_TASK();
    const auto& program = static_cast<const Program&>(GetProgram());
    const uint32_t frames_count = program.GetContext().GetType() == Rhi::IContext::Type::Render
                                ? dynamic_cast<const Base::RenderContext&>(program.GetContext()).GetSettings().frame_buffers_count
                                : 1U;

    // Count the number of constant and mutable descriptors to be allocated in each descriptor heap
    std::map<DescriptorHeap::Type, DescriptorsCountByAccess> descriptors_count_by_heap_type;
    for (const auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_ARG_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        // NOTE: addressable resource bindings do not require descriptors to be created, instead they use direct GPU memory offset from resource
        const auto& binding_settings = argument_binding_ptr->GetSettings();
        if (binding_settings.argument.IsAddressable())
            continue;

        const DescriptorHeap::Type           heap_type   = static_cast<const ArgumentBinding&>(*argument_binding_ptr).GetDescriptorHeapType();
        const Rhi::ProgramArgumentAccessor::Type access_type = binding_settings.argument.GetAccessorType();

        uint32_t resources_count = binding_settings.resource_count;
        if (access_type == Rhi::ProgramArgumentAccessor::Type::FrameConstant)
        {
            // For Frame Constant bindings we reserve descriptors range for all frames at once
            resources_count *= frames_count;
        }

        descriptors_count_by_heap_type[heap_type][access_type] += resources_count;
    }

    // Reserve descriptor ranges in heaps for resource bindings state
    auto& mutable_program = static_cast<Program&>(GetProgram());
    const DescriptorManager& descriptor_manager = mutable_program.GetDirectContext().GetDirectDescriptorManager();
    for (const auto& [heap_type, descriptors_count] : descriptors_count_by_heap_type)
    {
        std::optional<DescriptorHeap::Reservation>& descriptor_heap_reservation_opt = m_descriptor_heap_reservations_by_type[magic_enum::enum_integer(heap_type)];
        if (!descriptor_heap_reservation_opt)
        {
            descriptor_heap_reservation_opt.emplace(descriptor_manager.GetDefaultShaderVisibleDescriptorHeap(heap_type));
        }

        DescriptorHeap::Reservation& heap_reservation = *descriptor_heap_reservation_opt;
        META_CHECK_ARG_EQUAL(heap_reservation.heap.get().GetSettings().type, heap_type);
        META_CHECK_ARG_TRUE(heap_reservation.heap.get().GetSettings().shader_visible);

        for (Rhi::ProgramArgumentAccessor::Type access_type : magic_enum::enum_values<Rhi::ProgramArgumentAccessor::Type>())
        {
            const uint32_t accessor_descr_count = descriptors_count[access_type];
            if (!accessor_descr_count)
                continue;

            DescriptorHeap::Range& heap_range = heap_reservation.ranges[magic_enum::enum_index(access_type).value()];
            heap_range = mutable_program.ReserveDescriptorRange(heap_reservation.heap.get(), access_type, accessor_descr_count);

            if (access_type == Rhi::ProgramArgumentAccessor::Type::FrameConstant)
            {
                // Since Frame Constant binding range was reserved for all frames at once
                // we need to take only one sub-range related to the frame of current bindings
                const Data::Index frame_range_length = heap_range.GetLength() / frames_count;
                const Data::Index frame_range_start  = heap_range.GetStart() + frame_range_length * GetFrameIndex();
                heap_range = DescriptorHeap::Range(frame_range_start, frame_range_start + frame_range_length);
            }
        }
    }
}

void ProgramBindings::AddRootParameterBinding(const Rhi::ProgramArgumentAccessor& argument_accessor, const RootParameterBinding& root_parameter_binding)
{
    META_FUNCTION_TASK();
    m_root_parameter_bindings_by_access[argument_accessor.GetAccessorIndex()].emplace_back(root_parameter_binding);
}

void ProgramBindings::UpdateRootParameterBindings()
{
    META_FUNCTION_TASK();
    for(RootParameterBindings& root_parameter_bindings : m_root_parameter_bindings_by_access)
    {
        root_parameter_bindings.clear();
    }

    ForEachArgumentBinding([this](ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        AddRootParameterBindingsForArgument(argument_binding, p_heap_reservation);
    });
}

void ProgramBindings::AddRootParameterBindingsForArgument(ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
{
    META_FUNCTION_TASK();
    using DXBindingType     = ArgumentBinding::Type;
    using DXDescriptorRange = ArgumentBinding::DescriptorRange;

    const ArgumentBinding::Settings& binding_settings = argument_binding.GetDirectSettings();

    if (binding_settings.type == DXBindingType::DescriptorTable)
    {
        META_CHECK_ARG_NOT_NULL_DESCR(p_heap_reservation, "descriptor heap reservation is not available for \"Descriptor Table\" resource binding");
        const auto&              dx_descriptor_heap = static_cast<const DescriptorHeap&>(p_heap_reservation->heap.get());
        const DXDescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
        const uint32_t           descriptor_index   = p_heap_reservation->GetRange(binding_settings.argument.GetAccessorIndex()).GetStart() + descriptor_range.offset;

        AddRootParameterBinding(binding_settings.argument, {
            argument_binding,
            argument_binding.GetRootParameterIndex(),
            dx_descriptor_heap.GetNativeGpuDescriptorHandle(descriptor_index),
            0
        });
    }

    for (const ResourceView& resource_view_dx : argument_binding.GetDirectResourceViews())
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

void ProgramBindings::ApplyRootParameterBindings(Rhi::ProgramArgumentAccessor::Type access_types_mask, ID3D12GraphicsCommandList& d3d12_command_list,
                                                   const Base::ProgramBindings* applied_program_bindings_ptr, bool apply_changes_only) const
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    for(Rhi::ProgramArgumentAccessor::Type access_type : magic_enum::enum_values<Rhi::ProgramArgumentAccessor::Type>())
    {
        if (!static_cast<bool>(access_types_mask & access_type))
            continue;

        const bool do_program_bindings_comparing = access_type == Rhi::ProgramArgumentAccessor::Type::Mutable && apply_changes_only && applied_program_bindings_ptr;
        const RootParameterBindings& root_parameter_bindings = m_root_parameter_bindings_by_access[magic_enum::enum_index(access_type).value()];
        for (const RootParameterBinding& root_parameter_binding : root_parameter_bindings)
        {
            if (do_program_bindings_comparing && root_parameter_binding.argument_binding.IsAlreadyApplied(GetProgram(), *applied_program_bindings_ptr))
                continue;

            ApplyRootParameterBinding(root_parameter_binding, d3d12_command_list);
        }
    }
}

void ProgramBindings::ApplyRootParameterBinding(const RootParameterBinding& root_parameter_binding, ID3D12GraphicsCommandList& d3d12_command_list) const
{
    META_FUNCTION_TASK();
    switch (const ArgumentBinding::Type binding_type = root_parameter_binding.argument_binding.GetDirectSettings().type;
            binding_type)
    {
    case ArgumentBinding::Type::DescriptorTable:
        d3d12_command_list.SetGraphicsRootDescriptorTable(root_parameter_binding.root_parameter_index, root_parameter_binding.base_descriptor);
        break;

    case ArgumentBinding::Type::ConstantBufferView:
        d3d12_command_list.SetGraphicsRootConstantBufferView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    case ArgumentBinding::Type::ShaderResourceView:
        d3d12_command_list.SetGraphicsRootShaderResourceView(root_parameter_binding.root_parameter_index, root_parameter_binding.gpu_virtual_address);
        break;

    default:
        META_UNEXPECTED_ARG(binding_type);
    }
}

void ProgramBindings::CopyDescriptorsToGpu()
{
    META_FUNCTION_TASK();
    META_LOG("Copy descriptors to GPU for program bindings '{}'", GetName());

    const wrl::ComPtr<ID3D12Device>& cp_d3d12_device = static_cast<const Program&>(GetProgram()).GetDirectContext().GetDirectDevice().GetNativeDevice();
    ForEachArgumentBinding([this, &cp_d3d12_device](ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation)
    {
        CopyDescriptorsToGpuForArgument(cp_d3d12_device, argument_binding, p_heap_reservation);
    });
}

void ProgramBindings::CopyDescriptorsToGpuForArgument(const wrl::ComPtr<ID3D12Device>& d3d12_device, ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* p_heap_reservation) const
{
    META_FUNCTION_TASK();
    if (!p_heap_reservation)
        return;

    using AcceessType = Rhi::ProgramArgumentAccessor::Type;

    const auto&                               dx_descriptor_heap = static_cast<const DescriptorHeap&>(p_heap_reservation->heap.get());
    const ArgumentBinding::DescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
    const DescriptorHeap::Type              heap_type          = dx_descriptor_heap.GetSettings().type;
    const DescriptorHeap::Range&            heap_range         = p_heap_reservation->GetRange(argument_binding.GetSettings().argument.GetAccessorIndex());
    const D3D12_DESCRIPTOR_HEAP_TYPE          native_heap_type   = dx_descriptor_heap.GetNativeDescriptorHeapType();

    argument_binding.SetDescriptorHeapReservation(p_heap_reservation);
    META_CHECK_ARG_NOT_NULL(d3d12_device);
    META_CHECK_ARG_LESS_DESCR(descriptor_range.offset, heap_range.GetLength(),
                              "descriptor range offset is out of reserved descriptor range bounds");

    uint32_t resource_index = 0;
    for (const ResourceView& resource_view_dx : argument_binding.GetDirectResourceViews())
    {
        if (!resource_view_dx.HasDescriptor())
            continue;

        META_CHECK_ARG_EQUAL_DESCR(heap_type, resource_view_dx.GetDescriptor()->heap.GetSettings().type,
                                   "can not create binding for resource on descriptor heap of incompatible type");

        const uint32_t descriptor_index = heap_range.GetStart() + descriptor_range.offset + resource_index;
        META_LOG("  - Resource '{}' binding with {} access has descriptor heap range [{}, {}), CPU descriptor index {}",
                 resource_view_dx.GetDirectResource().GetName(),
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

} // namespace Methane::Graphics::DirectX
