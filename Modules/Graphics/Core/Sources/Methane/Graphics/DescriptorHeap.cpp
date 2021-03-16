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

FILE: Methane/Graphics/DescriptorHeap.cpp
Descriptor Heap is a platform abstraction of DirectX 12 descriptor heaps

******************************************************************************/

#include "DescriptorHeap.h"
#include "ResourceBase.h"

#include <Methane/Data/RangeUtils.hpp>
#include <Methane/Exceptions.hpp>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

DescriptorHeap::Reservation::Reservation(const Ref<DescriptorHeap>& heap)
    : heap(heap)
{
    META_FUNCTION_TASK();
    std::fill(ranges.begin(), ranges.end(), DescriptorHeap::Range(0, 0));
}

DescriptorHeap::Reservation::Reservation(const Ref<DescriptorHeap>& heap, const Ranges& ranges)
    : heap(heap)
    , ranges(ranges)
{
    META_FUNCTION_TASK();
}

DescriptorHeap::DescriptorHeap(ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
    , m_deferred_size(settings.size)
{
    META_FUNCTION_TASK();

    if (m_deferred_size > 0)
    {
        m_resources.reserve(m_deferred_size);
        m_free_ranges.Add({ 0, m_deferred_size });
    }
}

DescriptorHeap::~DescriptorHeap()
{
    META_FUNCTION_TASK();

    std::scoped_lock lock_guard(m_modification_mutex);

    // All descriptor ranges must be released when heap is destroyed
    assert((!m_deferred_size && m_free_ranges.IsEmpty()) ||
             m_free_ranges == RangeSet({ { 0, m_deferred_size } }));
}

Data::Index DescriptorHeap::AddResource(const ResourceBase& resource)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    if (!m_settings.deferred_allocation)
    {
        META_CHECK_ARG_LESS_DESCR(m_resources.size(), m_settings.size + 1,
                                  "{} descriptor heap is full, no free space to add a resource",
                                  magic_enum::enum_name(m_settings.type));
    }
    else if (m_resources.size() >= m_settings.size)
    {
        m_deferred_size++;
        Allocate();
    }

    m_resources.push_back(&resource);

    const auto resource_index = static_cast<Data::Index>(m_resources.size() - 1);
    m_free_ranges.Remove(Range(resource_index, resource_index + 1));

    return static_cast<int32_t>(resource_index);
}

Data::Index DescriptorHeap::ReplaceResource(const ResourceBase& resource, Data::Index at_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    META_CHECK_ARG_LESS(at_index, m_resources.size());
    m_resources[at_index] = &resource;
    return at_index;
}

void DescriptorHeap::RemoveResource(Data::Index at_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);

    META_CHECK_ARG_LESS(at_index, m_resources.size());
    m_resources[at_index] = nullptr;
    m_free_ranges.Add(Range(at_index, at_index + 1));
}

void DescriptorHeap::Allocate()
{
    META_FUNCTION_TASK();
    m_allocated_size = m_deferred_size;
    Emit(&IDescriptorHeapCallback::OnDescriptorHeapAllocated, std::ref(*this));
}

DescriptorHeap::Range DescriptorHeap::ReserveRange(Data::Size length)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_ZERO_DESCR(length, "unable to reserve empty descriptor range");
    std::scoped_lock lock_guard(m_modification_mutex);

    if (const Range reserved_range = Data::ReserveRange(m_free_ranges, length);
        reserved_range || !m_settings.deferred_allocation)
        return reserved_range;

    Range deferred_range(m_deferred_size, m_deferred_size + length);
    m_deferred_size += length;
    return deferred_range;
}

void DescriptorHeap::ReleaseRange(const Range& range)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_modification_mutex);
    m_free_ranges.Add(range);
}

void DescriptorHeap::SetDeferredAllocation(bool deferred_allocation)
{
    META_FUNCTION_TASK();
    m_settings.deferred_allocation = deferred_allocation;
}

} // namespace Methane::Graphics