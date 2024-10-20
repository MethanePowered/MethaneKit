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
#include <Methane/Graphics/DirectX/ICommandList.h>
#include <Methane/Graphics/DirectX/DescriptorHeap.h>
#include <Methane/Graphics/DirectX/DescriptorManager.h>

#include <Methane/Graphics/Base/CommandList.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Data/EnumMaskUtil.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::DirectX
{

DescriptorsCountByAccess::DescriptorsCountByAccess()
{
    std::fill(m_count_by_access_type.begin(), m_count_by_access_type.end(), 0U);
}

uint32_t& DescriptorsCountByAccess::operator[](Rhi::ProgramArgumentAccessType access_type)
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

uint32_t DescriptorsCountByAccess::operator[](Rhi::ProgramArgumentAccessType access_type) const
{
    return m_count_by_access_type[magic_enum::enum_index(access_type).value()];
}

ProgramBindings::ProgramBindings(Program& program, const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index)
    : Base::ProgramBindings(program, binding_value_by_argument, frame_index)
{
    META_FUNCTION_TASK();
    ReserveDescriptorHeapRanges();
}

ProgramBindings::ProgramBindings(const ProgramBindings& other_program_bindings, const BindingValueByArgument& replace_resource_views_by_argument, const Opt<Data::Index>& frame_index)
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

        if (const DescriptorHeap::Range& mutable_descriptor_range = heap_reservation_opt->ranges[magic_enum::enum_index(Rhi::ProgramArgumentAccessType::Mutable).value()];
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
    Base::ProgramBindings::Initialize();

    const auto& program = static_cast<Program&>(GetProgram());
    if (program.GetDirectContext().GetDirectDescriptorManager().IsDeferredHeapAllocation())
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

Ptr<Rhi::IProgramBindings> ProgramBindings::CreateCopy(const BindingValueByArgument& replace_binding_value_by_argument,
                                                       const Opt<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    auto program_bindings_ptr = std::make_shared<ProgramBindings>(*this, replace_binding_value_by_argument, frame_index);
    program_bindings_ptr->Initialize();
    return program_bindings_ptr;
}

void ProgramBindings::Apply(Base::CommandList& command_list, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    Apply(dynamic_cast<ICommandList&>(command_list), command_list.GetProgramBindingsPtr(), apply_behavior);
}

void ProgramBindings::Apply(ICommandList& command_list, const Base::ProgramBindings* applied_program_bindings_ptr, ApplyBehaviorMask apply_behavior) const
{
    META_FUNCTION_TASK();
    ReleaseRetainedRootConstantBuffers();

    Rhi::ProgramArgumentAccessMask apply_access_mask;
    apply_access_mask.SetBitOn(Rhi::ProgramArgumentAccessType::Mutable);

    if (!apply_behavior.HasAnyBit(ApplyBehavior::ConstantOnce) || !applied_program_bindings_ptr)
    {
        apply_access_mask.SetBitOn(Rhi::ProgramArgumentAccessType::Constant);
        apply_access_mask.SetBitOn(Rhi::ProgramArgumentAccessType::FrameConstant);
    }

    // Set resource transition barriers before applying resource bindings
    if (apply_behavior.HasAnyBit(ApplyBehavior::StateBarriers))
    {
        ApplyResourceTransitionBarriers(command_list, apply_access_mask);
    }

    // Apply root parameter bindings after resource barriers
    ApplyRootParameterBindings(apply_access_mask, command_list, applied_program_bindings_ptr,
                               apply_behavior.HasAnyBit(ApplyBehavior::ChangesOnly));
}

template<typename FuncType>
void ProgramBindings::ForEachArgumentBinding(FuncType argument_binding_function) const
{
    META_FUNCTION_TASK();
    for (auto& [program_argument, argument_binding_ptr] : GetArgumentBindings())
    {
        META_CHECK_NOT_NULL(argument_binding_ptr);
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
        META_CHECK_NOT_NULL_DESCR(argument_binding_ptr, "no resource binding is set for program argument '{}'", program_argument.GetName());

        // NOTE: addressable resource bindings do not require descriptors to be created, instead they use direct GPU memory offset from resource
        const auto& binding_settings = argument_binding_ptr->GetSettings();
        if (binding_settings.argument.IsAddressable())
            continue;

        const DescriptorHeap::Type           heap_type   = static_cast<const ArgumentBinding&>(*argument_binding_ptr).GetDescriptorHeapType();
        const Rhi::ProgramArgumentAccessType access_type = binding_settings.argument.GetAccessorType();

        uint32_t resources_count = binding_settings.resource_count;
        if (access_type == Rhi::ProgramArgumentAccessType::FrameConstant)
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
        META_CHECK_EQUAL(heap_reservation.heap.get().GetSettings().type, heap_type);
        META_CHECK_TRUE(heap_reservation.heap.get().GetSettings().shader_visible);

        for (Rhi::ProgramArgumentAccessType access_type : magic_enum::enum_values<Rhi::ProgramArgumentAccessType>())
        {
            const uint32_t accessor_descr_count = descriptors_count[access_type];
            if (!accessor_descr_count)
                continue;

            DescriptorHeap::Range& heap_range = heap_reservation.ranges[magic_enum::enum_index(access_type).value()];
            heap_range = mutable_program.ReserveDescriptorRange(heap_reservation.heap.get(), access_type, accessor_descr_count);

            if (access_type == Rhi::ProgramArgumentAccessType::FrameConstant)
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

    ForEachArgumentBinding([this](ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* heap_reservation_ptr)
    {
        AddRootParameterBindingsForArgument(argument_binding, heap_reservation_ptr);
    });
}

void ProgramBindings::AddRootParameterBindingsForArgument(ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* heap_reservation_ptr)
{
    META_FUNCTION_TASK();
    using DXBindingType     = ArgumentBinding::Type;
    using DXDescriptorRange = ArgumentBinding::DescriptorRange;

    if (const ArgumentBinding::Settings& binding_settings = argument_binding.GetDirectSettings();
        binding_settings.type == DXBindingType::DescriptorTable)
    {
        META_CHECK_NOT_NULL_DESCR(heap_reservation_ptr, "descriptor heap reservation is not available for \"Descriptor Table\" resource binding");
        const auto&              dx_descriptor_heap = static_cast<const DescriptorHeap&>(heap_reservation_ptr->heap.get());
        const DXDescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
        const uint32_t           descriptor_index   = heap_reservation_ptr->GetRange(binding_settings.argument.GetAccessorIndex()).GetStart() + descriptor_range.offset;

        AddRootParameterBinding(binding_settings.argument, {
            argument_binding,
            argument_binding.GetRootParameterIndex(),
            dx_descriptor_heap.GetNativeGpuDescriptorHandle(descriptor_index)
        });
    }
    else if (argument_binding.GetSettings().argument.IsRootConstantValue())
    {
        AddRootParameterBinding(binding_settings.argument, {
            argument_binding,
            argument_binding.GetRootParameterIndex()
        });
    }
    else
    {
        for (const ResourceView& resource_view_dx : argument_binding.GetDirectResourceViews())
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

void ProgramBindings::ApplyRootParameterBindings(Rhi::ProgramArgumentAccessMask access, const ICommandList& command_list,
                                                 const Base::ProgramBindings* applied_program_bindings_ptr, bool apply_changes_only) const
{
    META_FUNCTION_TASK();
    ID3D12GraphicsCommandList& d3d12_command_list = command_list.GetNativeCommandList();
    switch(Rhi::CommandListType command_list_type = command_list.GetCommandListType();
           command_list_type)
    {
    case Rhi::CommandListType::Render:
        ApplyRootParameterBindings<Rhi::CommandListType::Render>(access, d3d12_command_list, applied_program_bindings_ptr, apply_changes_only);
        break;

    case Rhi::CommandListType::Compute:
        ApplyRootParameterBindings<Rhi::CommandListType::Compute>(access, d3d12_command_list, applied_program_bindings_ptr, apply_changes_only);
        break;

    default:
        META_UNEXPECTED(command_list_type);
    }
}

template<Rhi::CommandListType command_list_type>
void ProgramBindings::ApplyRootParameterBindings(Rhi::ProgramArgumentAccessMask access, ID3D12GraphicsCommandList& d3d12_command_list,
                                                 const Base::ProgramBindings* applied_program_bindings_ptr, bool apply_changes_only) const
{
    META_FUNCTION_TASK();
    Data::ForEachBitInEnumMask(access,
        [this, &d3d12_command_list, applied_program_bindings_ptr, apply_changes_only](Rhi::ProgramArgumentAccessType access_type)
        {
           const bool do_program_bindings_comparing = access_type == Rhi::ProgramArgumentAccessType::Mutable && apply_changes_only && applied_program_bindings_ptr;
           const RootParameterBindings& root_parameter_bindings = m_root_parameter_bindings_by_access[magic_enum::enum_index(access_type).value()];

           for (const RootParameterBinding& root_parameter_binding : root_parameter_bindings)
           {
               if (do_program_bindings_comparing && root_parameter_binding.argument_binding.IsAlreadyApplied(GetProgram(), *applied_program_bindings_ptr))
                   continue;

               root_parameter_binding.Apply<command_list_type>(d3d12_command_list);
           }
        });
}

template<Rhi::CommandListType command_list_type>
void ProgramBindings::RootParameterBinding::Apply(ID3D12GraphicsCommandList& d3d12_command_list) const
{
    META_FUNCTION_TASK();
    switch (const ArgumentBinding::Type binding_type = argument_binding.GetDirectSettings().type;
            binding_type)
    {
    case ArgumentBinding::Type::DescriptorTable:
        if constexpr (command_list_type == Rhi::CommandListType::Render)
            d3d12_command_list.SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
        else if constexpr (command_list_type == Rhi::CommandListType::Compute)
            d3d12_command_list.SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
        break;

    case ArgumentBinding::Type::ConstantBufferView:
        if constexpr (command_list_type == Rhi::CommandListType::Render)
            d3d12_command_list.SetGraphicsRootConstantBufferView(root_parameter_index, gpu_virtual_address);
        else if constexpr (command_list_type == Rhi::CommandListType::Compute)
            d3d12_command_list.SetComputeRootConstantBufferView(root_parameter_index, gpu_virtual_address);
        break;

    case ArgumentBinding::Type::ShaderResourceView:
        if constexpr (command_list_type == Rhi::CommandListType::Render)
            d3d12_command_list.SetGraphicsRootShaderResourceView(root_parameter_index, gpu_virtual_address);
        else if constexpr (command_list_type == Rhi::CommandListType::Compute)
            d3d12_command_list.SetComputeRootShaderResourceView(root_parameter_index, gpu_virtual_address);
        break;

    case ArgumentBinding::Type::UnorderedAccessView:
        if constexpr (command_list_type == Rhi::CommandListType::Render)
            d3d12_command_list.SetGraphicsRootUnorderedAccessView(root_parameter_index, gpu_virtual_address);
        else if constexpr (command_list_type == Rhi::CommandListType::Compute)
            d3d12_command_list.SetComputeRootUnorderedAccessView(root_parameter_index, gpu_virtual_address);
        break;

    case ArgumentBinding::Type::Constant32Bit:
        if constexpr (command_list_type == Rhi::CommandListType::Render)
            d3d12_command_list.SetGraphicsRoot32BitConstants(root_parameter_index,
                                                             argument_binding.GetRootConstant().GetDataSize<UINT>(),
                                                             argument_binding.GetRootConstant().GetDataPtr<UINT>(),
                                                             0U);
        else if constexpr (command_list_type == Rhi::CommandListType::Compute)
            d3d12_command_list.SetComputeRoot32BitConstants(root_parameter_index,
                                                            argument_binding.GetRootConstant().GetDataSize<UINT>(),
                                                            argument_binding.GetRootConstant().GetDataPtr<UINT>(),
                                                            0U);
        break;

    default:
        META_UNEXPECTED(binding_type);
    }
}

void ProgramBindings::CopyDescriptorsToGpu() const
{
    META_FUNCTION_TASK();
    META_LOG("Copy descriptors to GPU for program bindings '{}'", GetName());

    const wrl::ComPtr<ID3D12Device>& d3d12_device_cptr = static_cast<const Program&>(GetProgram()).GetDirectContext().GetDirectDevice().GetNativeDevice();
    ForEachArgumentBinding([this, &d3d12_device_cptr](ArgumentBinding& argument_binding, const DescriptorHeap::Reservation* heap_reservation_ptr)
    {
        CopyDescriptorsToGpuForArgument(d3d12_device_cptr, argument_binding, heap_reservation_ptr);
    });
}

void ProgramBindings::CopyDescriptorsToGpuForArgument(const wrl::ComPtr<ID3D12Device>& d3d12_device,
                                                      ArgumentBinding& argument_binding,
                                                      const DescriptorHeap::Reservation* heap_reservation_ptr) const
{
    META_FUNCTION_TASK();
    if (!heap_reservation_ptr)
        return;

    using AcceessType = Rhi::ProgramArgumentAccessType;

    const auto&                             dx_descriptor_heap = static_cast<const DescriptorHeap&>(heap_reservation_ptr->heap.get());
    const ArgumentBinding::DescriptorRange& descriptor_range   = argument_binding.GetDescriptorRange();
    const DescriptorHeap::Type              heap_type          = dx_descriptor_heap.GetSettings().type;
    const DescriptorHeap::Range&            heap_range         = heap_reservation_ptr->GetRange(argument_binding.GetSettings().argument.GetAccessorIndex());
    const D3D12_DESCRIPTOR_HEAP_TYPE        native_heap_type   = dx_descriptor_heap.GetNativeDescriptorHeapType();

    argument_binding.SetDescriptorHeapReservation(heap_reservation_ptr);
    META_CHECK_NOT_NULL(d3d12_device);
    META_CHECK_LESS_DESCR(descriptor_range.offset, heap_range.GetLength(),
                          "descriptor range offset is out of reserved descriptor range bounds");

    uint32_t resource_index = 0;
    for (const ResourceView& resource_view_dx : argument_binding.GetDirectResourceViews())
    {
        if (!resource_view_dx.HasDescriptor())
            continue;

        META_CHECK_EQUAL_DESCR(heap_type, resource_view_dx.GetDescriptor()->heap.GetSettings().type,
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
