/******************************************************************************

Copyright 2020-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ResourceBarriers.cpp
Methane resource barriers base implementation.

******************************************************************************/

#include <Methane/Graphics/Base/ResourceBarriers.h>

#include <Methane/Graphics/IResource.h>
#include <Methane/Graphics/ICommandQueue.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::Base
{

ResourceBarriers::ResourceBarriers(const Set& barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::inserter(m_barriers_map, m_barriers_map.begin()),
                   [](const ResourceBarrier& barrier)
                   { return std::pair<ResourceBarrierId, ResourceBarrier>(barrier.GetId(), barrier); });
}

ResourceBarriers::Set ResourceBarriers::GetSet() const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    Set barriers;
    std::transform(m_barriers_map.begin(), m_barriers_map.end(), std::inserter(barriers, barriers.begin()),
                   [](const auto& barrier_pair) { return barrier_pair.second; });
    return barriers;
}

const ResourceBarrier* ResourceBarriers::GetBarrier(const ResourceBarrierId& id) const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(id);
    return barrier_it == m_barriers_map.end() ? nullptr : &barrier_it->second;
}

bool ResourceBarriers::HasStateTransition(IResource& resource, ResourceState before, ResourceState after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(ResourceBarrierId(ResourceBarrier::Type::StateTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == ResourceBarrier(resource, before, after);
}

bool ResourceBarriers::HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(ResourceBarrierId(ResourceBarrier::Type::OwnerTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == ResourceBarrier(resource, queue_family_before, queue_family_after);
}

ResourceBarriers::AddResult ResourceBarriers::AddStateTransition(IResource& resource, ResourceState before, ResourceState after)
{
    return Add(ResourceBarrierId(ResourceBarrier::Type::StateTransition, resource), ResourceBarrier(resource, before, after));
}

ResourceBarriers::AddResult ResourceBarriers::AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
{
    return Add(ResourceBarrierId(ResourceBarrier::Type::OwnerTransition, resource), ResourceBarrier(resource, queue_family_before, queue_family_after));
}

bool ResourceBarriers::Remove(ResourceBarrier::Type type, IResource& resource)
{
    return Remove(ResourceBarrierId(type, resource));
}

bool ResourceBarriers::RemoveStateTransition(IResource& resource)
{
    return Remove(ResourceBarrierId(ResourceBarrier::Type::StateTransition, resource));
}

bool ResourceBarriers::RemoveOwnerTransition(IResource& resource)
{
    return Remove(ResourceBarrierId(ResourceBarrier::Type::OwnerTransition, resource));
}

ResourceBarriers::AddResult ResourceBarriers::Add(const ResourceBarrierId& id, const ResourceBarrier& barrier)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);

    const auto [ barrier_id_and_state_change_it, barrier_added ] = m_barriers_map.try_emplace(id, barrier);
    if (barrier_added)
        return AddResult::Added;

    if (barrier_id_and_state_change_it->second == barrier)
        return AddResult::Existing;

    barrier_id_and_state_change_it->second = barrier;
    return AddResult::Updated;
}

bool ResourceBarriers::Remove(const ResourceBarrierId& id)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    return m_barriers_map.erase(id);
}

void ResourceBarriers::ApplyTransitions() const
{
    META_FUNCTION_TASK();
    for(const auto& [barrier_id, barrier] : m_barriers_map)
    {
         barrier.ApplyTransition();
    }
}

ResourceBarriers::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    std::stringstream ss;
    for(auto barrier_pair_it = m_barriers_map.begin(); barrier_pair_it != m_barriers_map.end(); ++barrier_pair_it)
    {
        ss << "  - " << static_cast<std::string>(barrier_pair_it->second);
        if (barrier_pair_it != std::prev(m_barriers_map.end()))
            ss << ";" << std::endl;
        else
            ss << ".";
    }
    return ss.str();
}

} // namespace Methane::Graphics::Base