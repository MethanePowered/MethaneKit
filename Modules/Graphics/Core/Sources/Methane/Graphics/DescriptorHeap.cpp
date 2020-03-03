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

FILE: Methane/Graphics/DescriptorHeap.cpp
Descriptor Heap is a platform abstraction of DirectX 12 descriptor heaps

******************************************************************************/

#include "DescriptorHeap.h"
#include "ResourceBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

DescriptorHeap::Reservation::Reservation(const Ref<DescriptorHeap>& in_heap, const Range& in_constant_range, const Range& in_mutable_range)
    : heap(in_heap)
    , constant_range(in_constant_range)
    , mutable_range(in_mutable_range)
{
    ITT_FUNCTION_TASK();
}

DescriptorHeap::DescriptorHeap(ContextBase& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
    , m_deferred_size(settings.size)
{
    ITT_FUNCTION_TASK();

    if (m_deferred_size > 0)
    {
        m_resources.reserve(m_deferred_size);
        m_free_ranges.Add({ 0, m_deferred_size });
    }
}

DescriptorHeap::~DescriptorHeap()
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_modification_mutex);

    // All descriptor ranges must be released when heap is destroyed
    assert((!m_deferred_size && m_free_ranges.IsEmpty()) ||
             m_free_ranges == RangeSet({ { 0, m_deferred_size } }));
}

Data::Index DescriptorHeap::AddResource(const ResourceBase& resource)
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_modification_mutex);

    if (m_resources.size() >= m_settings.size)
    {
        if (m_settings.deferred_allocation)
        {
            m_deferred_size++;
            Allocate();
        }
        else
        {
            throw std::runtime_error("\"" + GetTypeName() + "\" descriptor heap is full (size: " + std::to_string(m_settings.size) +
                                     "), no free space to add a resource.");
        }
    }

    m_resources.push_back(&resource);

    const Data::Index resource_index = static_cast<Data::Index>(m_resources.size() - 1);
    m_free_ranges.Remove(Range(resource_index, resource_index + 1));

    return static_cast<int32_t>(resource_index);
}

Data::Index DescriptorHeap::ReplaceResource(const ResourceBase& resource, Data::Index at_index)
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_modification_mutex);

    if (at_index >= m_resources.size())
    {
        throw std::runtime_error("Index " + std::to_string(at_index) + 
                                 " is out of " + GetTypeName() + " descriptor heap bounds.");
    }

    m_resources[at_index] = &resource;
    return at_index;
}

void DescriptorHeap::RemoveResource(Data::Index at_index)
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_modification_mutex);

    if (at_index >= m_resources.size())
    {
        throw std::runtime_error("Can not remove resource: index (" + std::to_string(at_index) + 
                                 ") is out of \"" + GetTypeName() + "\" descriptor heap bounds.");
    }

    m_resources[at_index] = nullptr;
    m_free_ranges.Add(Range(at_index, at_index + 1));
}

Ptr<DescriptorHeap::Range> DescriptorHeap::ReserveRange(Data::Size length)
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_modification_mutex);

    RangeSet::ConstIterator free_range_it = std::find_if(m_free_ranges.begin(), m_free_ranges.end(),
        [length](const Range& range)
        {
            return range.GetLength() >= length;
        }
    );

    if (free_range_it == m_free_ranges.end())
    {
        if (m_settings.deferred_allocation)
        {
            Ptr<Range> sp_reserved_range(new Range(m_deferred_size, m_deferred_size + length));
            m_deferred_size += length;
            return sp_reserved_range;
        }
        else
            return nullptr;
    }
    
    Ptr<Range> sp_reserved_range(new Range(free_range_it->GetStart(), free_range_it->GetStart() + length));
    m_free_ranges.Remove(*sp_reserved_range);

    return sp_reserved_range;
}

void DescriptorHeap::ReleaseRange(const Range& range)
{
    ITT_FUNCTION_TASK();

    std::lock_guard<std::mutex> lock_guard(m_modification_mutex);
    m_free_ranges.Add(range);
}

std::string DescriptorHeap::GetTypeName(Type heap_type)
{
    ITT_FUNCTION_TASK();
    switch (heap_type)
    {
        case Type::ShaderResources: return "ShaderBase Resources";
        case Type::Samplers:        return "Samplers";
        case Type::RenderTargets:   return "Render Targets";
        case Type::DepthStencil:    return "Depth Stencil";
        default:                    return "Undefined";
    }
}

} // namespace Methane::Graphics