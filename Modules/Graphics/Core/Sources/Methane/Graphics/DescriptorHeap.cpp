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

#include <Methane/Data/RangeUtils.hpp>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

DescriptorHeap::Reservation::Reservation(const Ref<DescriptorHeap>& in_heap, const Range& in_constant_range, const Range& in_mutable_range)
    : heap(in_heap)
    , constant_range(in_constant_range)
    , mutable_range(in_mutable_range)
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

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_modification_mutex);

    // All descriptor ranges must be released when heap is destroyed
    assert((!m_deferred_size && m_free_ranges.IsEmpty()) ||
             m_free_ranges == RangeSet({ { 0, m_deferred_size } }));
}

Data::Index DescriptorHeap::AddResource(const ResourceBase& resource)
{
    META_FUNCTION_TASK();

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_modification_mutex);

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
    META_FUNCTION_TASK();

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_modification_mutex);

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
    META_FUNCTION_TASK();

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_modification_mutex);

    if (at_index >= m_resources.size())
    {
        throw std::runtime_error("Can not remove resource: index (" + std::to_string(at_index) + 
                                 ") is out of \"" + GetTypeName() + "\" descriptor heap bounds.");
    }

    m_resources[at_index] = nullptr;
    m_free_ranges.Add(Range(at_index, at_index + 1));
}

void DescriptorHeap::Allocate()
{
    META_FUNCTION_TASK();
    m_allocated_size = m_deferred_size;
    Notify(Notification::Allocated);
}

void DescriptorHeap::AddNotification(const ObjectBase& target, NotificationCallback notification)
{
    META_FUNCTION_TASK();
    auto notification_callback_it = std::find_if(m_notification_callbacks.begin(), m_notification_callbacks.end(),
        [target](const auto& notification_callback)
        { return notification_callback.first == std::addressof(target); }
    );
    if (notification_callback_it == m_notification_callbacks.end())
    {
        m_notification_callbacks.emplace_back(&target, std::move(notification));
    }
    else
    {
        notification_callback_it->second = std::move(notification);
    }
}

void DescriptorHeap::RemoveNotification(const ObjectBase& target)
{
    META_FUNCTION_TASK();
    auto notification_callback_it = std::find_if(m_notification_callbacks.begin(), m_notification_callbacks.end(),
         [target](const auto& notification_callback)
         { return notification_callback.first == std::addressof(target); }
    );
    if (notification_callback_it == m_notification_callbacks.end())
        return;

    m_notification_callbacks.erase(notification_callback_it);
}

void DescriptorHeap::Notify(Notification notification)
{
    META_FUNCTION_TASK();
    for(const auto& notification_callback : m_notification_callbacks)
    {
        notification_callback.second(*this, notification);
    }
}

DescriptorHeap::Range DescriptorHeap::ReserveRange(Data::Size length)
{
    META_FUNCTION_TASK();
    if (!length)
        throw std::invalid_argument("Unable to reserve empty descriptor range.");

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_modification_mutex);

    Range reserved_range = Data::ReserveRange(m_free_ranges, length);
    if (reserved_range || !m_settings.deferred_allocation)
        return reserved_range;

    Range deferred_range(m_deferred_size, m_deferred_size + length);
    m_deferred_size += length;
    return deferred_range;
}

void DescriptorHeap::ReleaseRange(const Range& range)
{
    META_FUNCTION_TASK();

    std::lock_guard<LockableBase(std::mutex)> lock_guard(m_modification_mutex);
    m_free_ranges.Add(range);
}

void DescriptorHeap::SetDeferredAllocation(bool deferred_allocation)
{
    META_FUNCTION_TASK();
    m_settings.deferred_allocation = deferred_allocation;
}

std::string DescriptorHeap::GetTypeName(Type heap_type)
{
    META_FUNCTION_TASK();
    switch (heap_type)
    {
        case Type::ShaderResources: return "Shader Resources";
        case Type::Samplers:        return "Samplers";
        case Type::RenderTargets:   return "Render Targets";
        case Type::DepthStencil:    return "Depth Stencil";
        default:                    return "Undefined";
    }
}

} // namespace Methane::Graphics