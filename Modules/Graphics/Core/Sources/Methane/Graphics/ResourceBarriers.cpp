/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/ResourceBarriers.cpp
Methane resource barriers for manual or automatic resource state synchronization on GPU.

******************************************************************************/

#include <Methane/Graphics/ResourceBarriers.h>
#include <Methane/Graphics/Resource.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

ResourceBarrier::Id::Id(Type type, const Resource& resource) noexcept
    : m_type(type)
    , m_resource_ref(resource)
{
    META_FUNCTION_TASK();
}

bool ResourceBarrier::Id::operator<(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    const Resource* p_this_resource  = std::addressof(m_resource_ref.get());
    const Resource* p_other_resource = std::addressof(other.GetResource());
    return std::tie(m_type, p_this_resource) < std::tie(other.m_type, p_other_resource);
}

bool ResourceBarrier::Id::operator==(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    const Resource* p_this_resource  = std::addressof(m_resource_ref.get());
    const Resource* p_other_resource = std::addressof(other.GetResource());
    return std::tie(m_type, p_this_resource) == std::tie(other.m_type, p_other_resource);
}

bool ResourceBarrier::Id::operator!=(const Id& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBarrier::StateChange::StateChange(ResourceState before, ResourceState after) noexcept
    : m_before(before)
    , m_after(after)
{
    META_FUNCTION_TASK();
}

bool ResourceBarrier::StateChange::operator<(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_before, m_after) < std::tie(other.m_before, other.m_after);
}

bool ResourceBarrier::StateChange::operator==(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_before, m_after) == std::tie(other.m_before, other.m_after);
}

bool ResourceBarrier::StateChange::operator!=(const StateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBarrier::ResourceBarrier(const Id& id, const StateChange& state_change)
    : m_id(id)
    , m_state_change(state_change)
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(Type type, const Resource& resource, ResourceState state_before, ResourceState state_after)
    : ResourceBarrier(Id(type, resource), StateChange(state_before, state_after))
{
    META_FUNCTION_TASK();
}


bool ResourceBarrier::operator<(const ResourceBarrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_id, m_state_change) < std::tie(other.m_id, other.m_state_change);
}

bool ResourceBarrier::operator==(const ResourceBarrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_id, m_state_change) == std::tie(other.m_id, other.m_state_change);
}

bool ResourceBarrier::operator!=(const ResourceBarrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBarrier::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    return fmt::format("Resource '{}' {} barrier from {} to {} state",
                       m_id.GetResource().GetName(),
                       magic_enum::enum_name(m_id.GetType()),
                       magic_enum::enum_name(m_state_change.GetStateBefore()),
                       magic_enum::enum_name(m_state_change.GetStateAfter()));
}

Ptr<ResourceBarriers> ResourceBarriers::CreateTransition(const Refs<const Resource>& resources, ResourceState state_before, ResourceState state_after)
{
    META_FUNCTION_TASK();
    std::set<ResourceBarrier> resource_barriers;
    for (const Ref<const Resource>& resource_ref : resources)
    {
        resource_barriers.emplace(
            ResourceBarrier::Type::Transition,
            resource_ref.get(),
            state_before,
            state_after
        );
    }
    return ResourceBarriers::Create(resource_barriers);
}

ResourceBarriers::ResourceBarriers(const Set& barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::inserter(m_barriers_map, m_barriers_map.begin()),
                   [](const ResourceBarrier& barrier)
                       {
                           return std::pair<ResourceBarrier::Id, ResourceBarrier::StateChange>{ barrier.GetId(), barrier.GetStateChange() };
                       }
    );
}

ResourceBarriers::Set ResourceBarriers::GetSet() const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);

    Set barriers;
    std::transform(m_barriers_map.begin(), m_barriers_map.end(), std::inserter(barriers, barriers.begin()),
                   [](const auto& barrier_pair)
                       {
                           return ResourceBarrier(barrier_pair.first, barrier_pair.second);
                       }
    );
    return barriers;
}

bool ResourceBarriers::Has(ResourceBarrier::Type type, const Resource& resource, ResourceState before, ResourceState after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);

    const auto barrier_it = m_barriers_map.find(ResourceBarrier::Id(type, resource));
    if (barrier_it == m_barriers_map.end())
        return false;

    return barrier_it->second == ResourceBarrier::StateChange(before, after);
}

bool ResourceBarriers::HasTransition(const Resource& resource, ResourceState before, ResourceState after)
{
    META_FUNCTION_TASK();
    return Has(ResourceBarrier::Type::Transition, resource, before, after);
}

ResourceBarriers::AddResult ResourceBarriers::AddStateChange(const ResourceBarrier::Id& id, const ResourceBarrier::StateChange& state_change)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);

    const auto [ barrier_id_and_state_change_it, barrier_added ] = m_barriers_map.try_emplace(id, state_change);
    if (barrier_added)
        return AddResult::Added;

    if (barrier_id_and_state_change_it->second == state_change)
        return AddResult::Existing;

    barrier_id_and_state_change_it->second = state_change;
    return AddResult::Updated;
}

bool ResourceBarriers::Remove(const ResourceBarrier::Id& id)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    return m_barriers_map.erase(id);
}

void ResourceBarriers::UpdateResourceStates()
{
    META_FUNCTION_TASK();
    for(const auto& [barrier_id, state_change] : m_barriers_map)
    {
        META_CHECK_ARG_EQUAL_DESCR(barrier_id.GetResource().GetState(), state_change.GetStateBefore(),
                                   "state of resource '{}' does not match with transition barrier 'before' state", barrier_id.GetResource().GetName());
        const_cast<Resource&>(barrier_id.GetResource()).SetState(state_change.GetStateAfter());
    }
}

ResourceBarriers::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    std::stringstream ss;
    for(auto barrier_pair_it = m_barriers_map.begin(); barrier_pair_it != m_barriers_map.end(); ++barrier_pair_it)
    {
        ss << "  - " << static_cast<std::string>(ResourceBarrier(barrier_pair_it->first, barrier_pair_it->second));
        if (barrier_pair_it != std::prev(m_barriers_map.end()))
            ss << ";" << std::endl;
        else
            ss << ".";
    }
    return ss.str();
}

} // namespace Methane::Graphics