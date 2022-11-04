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

FILE: Methane/Graphics/DirectX12/ProgramArgumentBindingDX.cpp
DirectX 12 implementation of the program argument binding interface.

******************************************************************************/

#pragma once

#include "ProgramArgumentBindingDX.h"
#include "DeviceDX.h"

#include <Methane/Graphics/IContext.h>
#include <Methane/Graphics/ContextBase.h>

namespace Methane::Graphics
{

Ptr<ProgramArgumentBindingBase> ProgramArgumentBindingBase::CreateCopy(const ProgramArgumentBindingBase& other_argument_binding)
{
    META_FUNCTION_TASK();
    return std::make_shared<ProgramArgumentBindingDX>(static_cast<const ProgramArgumentBindingDX&>(other_argument_binding));
}

ProgramArgumentBindingDX::ProgramArgumentBindingDX(const ContextBase& context, const SettingsDX& settings)
    : ProgramArgumentBindingBase(context, settings)
    , m_settings_dx(settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME("m_p_descriptor_heap_reservation", !m_p_descriptor_heap_reservation);
}

ProgramArgumentBindingDX::ProgramArgumentBindingDX(const ProgramArgumentBindingDX& other)
    : ProgramArgumentBindingBase(other)
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

DescriptorHeapTypeDX ProgramArgumentBindingDX::GetDescriptorHeapType() const
{
    META_FUNCTION_TASK();
    return (GetSettings().resource_type == IResource::Type::Sampler)
           ? DescriptorHeapTypeDX::Samplers
           : DescriptorHeapTypeDX::ShaderResources;
}

bool ProgramArgumentBindingDX::SetResourceViews(const IResource::Views& resource_views)
{
    META_FUNCTION_TASK();
    if (!ProgramArgumentBindingBase::SetResourceViews(resource_views))
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
    for(const IResource::View& resource_view : resource_views)
    {
        m_resource_views_dx.emplace_back(resource_view, IResource::Usage::ShaderRead);
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

    GetContext().RequestDeferredAction(ContextDeferredAction::CompleteInitialization);
    return true;
}

void ProgramArgumentBindingDX::SetDescriptorRange(const DescriptorRange& descriptor_range)
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

void ProgramArgumentBindingDX::SetDescriptorHeapReservation(const DescriptorHeapReservationDX* p_reservation)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NAME_DESCR("p_reservation",
                              !p_reservation || (p_reservation->heap.get().IsShaderVisible() && p_reservation->heap.get().GetSettings().type == m_descriptor_range.heap_type),
                              "argument binding reservation must be made in shader visible descriptor heap of type '{}'",
                              magic_enum::enum_name(m_descriptor_range.heap_type));
    m_p_descriptor_heap_reservation = p_reservation;
}

} // namespace Methane::Graphics
