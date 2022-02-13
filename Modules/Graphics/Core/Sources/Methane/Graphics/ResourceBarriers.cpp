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

FILE: Methane/Graphics/ResourceBarriers.cpp
Methane resource barriers for manual or automatic resource state synchronization on GPU.

******************************************************************************/

#include <Methane/Graphics/ResourceBarriers.h>
#include <Methane/Graphics/Resource.h>
#include <Methane/Graphics/CommandQueue.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

ResourceBarrier::Id::Id(Type type, Resource& resource) noexcept
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

ResourceBarrier::OwnerChange::OwnerChange(CommandQueue& before, CommandQueue& after) noexcept
    : m_before(before)
    , m_after(after)
{
    META_FUNCTION_TASK();
}

bool ResourceBarrier::OwnerChange::operator<(const OwnerChange& other) const noexcept
{
    META_FUNCTION_TASK();
    CommandQueue* before_ptr = std::addressof(m_before.get());
    CommandQueue* after_ptr  = std::addressof(m_after.get());
    CommandQueue* other_before_ptr = std::addressof(other.m_before.get());
    CommandQueue* other_after_ptr  = std::addressof(other.m_after.get());
    return std::tie(before_ptr, after_ptr) <
           std::tie(other_before_ptr, other_after_ptr);
}

bool ResourceBarrier::OwnerChange::operator==(const OwnerChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::addressof(m_before.get()) == std::addressof(other.m_before.get()) &&
           std::addressof(m_after.get()) == std::addressof(other.m_after.get());
}

bool ResourceBarrier::OwnerChange::operator!=(const OwnerChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBarrier::ResourceBarrier(Resource& resource, const StateChange& state_change)
    : m_id(Type::StateTransition, resource)
    , m_change(state_change)
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(Resource& resource, const OwnerChange& owner_change)
    : m_id(Type::OwnerTransition, resource)
    , m_change(owner_change)
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(Resource& resource, ResourceState state_before, ResourceState state_after)
    : ResourceBarrier(resource, StateChange(state_before, state_after))
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(Resource& resource, CommandQueue& owner_before, CommandQueue& owner_after)
    : ResourceBarrier(resource, OwnerChange(owner_before, owner_after))
{
    META_FUNCTION_TASK();
}

bool ResourceBarrier::operator<(const ResourceBarrier& other) const noexcept
{
    META_FUNCTION_TASK();
    switch(m_id.GetType())
    {
    case Type::StateTransition: return std::tie(m_id, m_change.state) < std::tie(other.m_id, other.m_change.state);
    case Type::OwnerTransition: return std::tie(m_id, m_change.owner) < std::tie(other.m_id, other.m_change.owner);
    }
    return false;
}

bool ResourceBarrier::operator==(const ResourceBarrier& other) const noexcept
{
    META_FUNCTION_TASK();
    switch(m_id.GetType())
    {
    case Type::StateTransition: return std::tie(m_id, m_change.state) == std::tie(other.m_id, other.m_change.state);
    case Type::OwnerTransition: return std::tie(m_id, m_change.owner) == std::tie(other.m_id, other.m_change.owner);
    }
    return false;
}

bool ResourceBarrier::operator!=(const ResourceBarrier& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

bool ResourceBarrier::operator==(const StateChange& other_state_change) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(m_id.GetType(), Type::StateTransition);
    return m_change.state == other_state_change;
}

bool ResourceBarrier::operator!=(const StateChange& other_state_change) const
{
    META_FUNCTION_TASK();
    return !operator==(other_state_change);
}
bool ResourceBarrier::operator==(const OwnerChange& other_owner_change) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(m_id.GetType(), Type::StateTransition);
    return m_change.owner == other_owner_change;
}
bool ResourceBarrier::operator!=(const OwnerChange& other_owner_change) const
{
    META_FUNCTION_TASK();
    return !operator==(other_owner_change);
}

ResourceBarrier::operator std::string() const noexcept
{
    META_FUNCTION_TASK();
    switch(m_id.GetType())
    {
    case Type::StateTransition:
        return fmt::format("Resource '{}' state transition barrier from {} to {} state",
                           m_id.GetResource().GetName(),
                           magic_enum::enum_name(m_change.state.GetStateBefore()),
                           magic_enum::enum_name(m_change.state.GetStateAfter()));

    case Type::OwnerTransition:
        return fmt::format("Resource '{}' ownership transition barrier from '{}' to '{}' command queue",
                           m_id.GetResource().GetName(),
                           m_change.owner.GetOwnerBefore().GetName(),
                           m_change.owner.GetOwnerAfter().GetName());
    }
    return "";
}

const ResourceBarrier::StateChange& ResourceBarrier::GetStateChange() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(m_id.GetType(), ResourceBarrier::Type::StateTransition);
    return m_change.state;
}

const ResourceBarrier::OwnerChange& ResourceBarrier::GetOwnerChange() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(m_id.GetType(), ResourceBarrier::Type::OwnerTransition);
    return m_change.owner;
}

void ResourceBarrier::ApplyTransition() const
{
    META_FUNCTION_TASK();
    switch(m_id.GetType())
    {
    case Type::StateTransition:
        META_CHECK_ARG_EQUAL_DESCR(m_id.GetResource().GetState(), m_change.state.GetStateBefore(),
                                   "state of resource '{}' does not match with transition barrier 'before' state",
                                   m_id.GetResource().GetName());
        m_id.GetResource().SetState(m_change.state.GetStateAfter());
        break;

    case Type::OwnerTransition:
        META_CHECK_ARG_EQUAL_DESCR(m_id.GetResource().GetOwnerQueue(), &m_change.owner.GetOwnerBefore(),
                                   "owner of resource '{}' does not match with transition barrier 'before' state",
                                   m_id.GetResource().GetName());
        m_id.GetResource().SetOwnerQueue(m_change.owner.GetOwnerAfter());
        break;
    }
}

Ptr<ResourceBarriers> ResourceBarriers::CreateTransitions(const Refs<Resource>& resources,
                                                          const Opt<ResourceBarrier::StateChange>& state_change,
                                                          const Opt<ResourceBarrier::OwnerChange>& owner_change)
{
    META_FUNCTION_TASK();
    Set resource_barriers;
    for (const Ref<Resource>& resource_ref : resources)
    {
        if (owner_change.has_value())
            resource_barriers.emplace(resource_ref.get(), *owner_change);

        if (state_change.has_value())
            resource_barriers.emplace(resource_ref.get(), *state_change);
    }
    return ResourceBarriers::Create(resource_barriers);
}

ResourceBarriers::ResourceBarriers(const Set& barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::inserter(m_barriers_map, m_barriers_map.begin()),
                   [](const ResourceBarrier& barrier)
                   { return std::pair<ResourceBarrier::Id, ResourceBarrier>(barrier.GetId(), barrier); });
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

bool ResourceBarriers::HasStateTransition(Resource& resource, ResourceState before, ResourceState after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(ResourceBarrier::Id(ResourceBarrier::Type::StateTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == ResourceBarrier(resource, before, after);
}

bool ResourceBarriers::HasOwnerTransition(Resource& resource, CommandQueue& before, CommandQueue& after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(ResourceBarrier::Id(ResourceBarrier::Type::OwnerTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == ResourceBarrier(resource, before, after);
}

ResourceBarriers::AddResult ResourceBarriers::AddStateTransition(Resource& resource, ResourceState before, ResourceState after)
{
    return Add(ResourceBarrier::Id(ResourceBarrier::Type::StateTransition, resource), ResourceBarrier(resource, before, after));
}

ResourceBarriers::AddResult ResourceBarriers::AddOwnerTransition(Resource& resource, CommandQueue& before, CommandQueue& after)
{
    return Add(ResourceBarrier::Id(ResourceBarrier::Type::OwnerTransition, resource), ResourceBarrier(resource, before, after));
}

bool ResourceBarriers::Remove(ResourceBarrier::Type type, Resource& resource)
{
    return Remove(ResourceBarrier::Id(type, resource));
}

bool ResourceBarriers::RemoveStateTransition(Resource& resource)
{
    return Remove(ResourceBarrier::Id(ResourceBarrier::Type::StateTransition, resource));
}

bool ResourceBarriers::RemoveOwnerTransition(Resource& resource)
{
    return Remove(ResourceBarrier::Id(ResourceBarrier::Type::OwnerTransition, resource));
}

ResourceBarriers::AddResult ResourceBarriers::Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier)
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

bool ResourceBarriers::Remove(const ResourceBarrier::Id& id)
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

} // namespace Methane::Graphics