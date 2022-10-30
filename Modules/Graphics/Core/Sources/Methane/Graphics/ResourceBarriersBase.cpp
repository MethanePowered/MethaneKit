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

FILE: Methane/Graphics/ResourceBarriersBase.cpp
Methane resource barriers base implementation.

******************************************************************************/

#include "ResourceBarriersBase.h"

#include <Methane/Graphics/IResource.h>
#include <Methane/Graphics/ICommandQueue.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

ResourceBarrierId::ResourceBarrierId(Type type, IResource& resource) noexcept
    : m_type(type)
    , m_resource_ref(resource)
{
    META_FUNCTION_TASK();
}

bool ResourceBarrierId::operator<(const ResourceBarrierId& other) const noexcept
{
    META_FUNCTION_TASK();
    const IResource* p_this_resource  = std::addressof(m_resource_ref.get());
    const IResource* p_other_resource = std::addressof(other.GetResource());
    return std::tie(m_type, p_this_resource) < std::tie(other.m_type, p_other_resource);
}

bool ResourceBarrierId::operator==(const ResourceBarrierId& other) const noexcept
{
    META_FUNCTION_TASK();
    const IResource* p_this_resource  = std::addressof(m_resource_ref.get());
    const IResource* p_other_resource = std::addressof(other.GetResource());
    return std::tie(m_type, p_this_resource) == std::tie(other.m_type, p_other_resource);
}

bool ResourceBarrierId::operator!=(const ResourceBarrierId& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceStateChange::ResourceStateChange(ResourceState before, ResourceState after) noexcept
    : m_before(before)
    , m_after(after)
{
    META_FUNCTION_TASK();
}

bool ResourceStateChange::operator<(const ResourceStateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_before, m_after) < std::tie(other.m_before, other.m_after);
}

bool ResourceStateChange::operator==(const ResourceStateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_before, m_after) == std::tie(other.m_before, other.m_after);
}

bool ResourceStateChange::operator!=(const ResourceStateChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceOwnerChange::ResourceOwnerChange(uint32_t queue_family_before, uint32_t queue_family_after) noexcept
    : m_queue_family_before(queue_family_before)
    , m_queue_family_after(queue_family_after)
{
    META_FUNCTION_TASK();
}

bool ResourceOwnerChange::operator<(const ResourceOwnerChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_queue_family_before, m_queue_family_after) <
           std::tie(other.m_queue_family_before, other.m_queue_family_after);
}

bool ResourceOwnerChange::operator==(const ResourceOwnerChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return std::tie(m_queue_family_before, m_queue_family_after) ==
           std::tie(other.m_queue_family_before, other.m_queue_family_after);
}

bool ResourceOwnerChange::operator!=(const ResourceOwnerChange& other) const noexcept
{
    META_FUNCTION_TASK();
    return !operator==(other);
}

ResourceBarrier::ResourceBarrier(IResource& resource, const StateChange& state_change)
    : m_id(Type::StateTransition, resource)
    , m_change(state_change)
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(IResource& resource, const OwnerChange& owner_change)
    : m_id(Type::OwnerTransition, resource)
    , m_change(owner_change)
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(IResource& resource, ResourceState state_before, ResourceState state_after)
    : ResourceBarrier(resource, StateChange(state_before, state_after))
{
    META_FUNCTION_TASK();
}

ResourceBarrier::ResourceBarrier(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
    : ResourceBarrier(resource, OwnerChange(queue_family_before, queue_family_after))
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
        return fmt::format("Resource '{}' ownership transition barrier from '{}' to '{}' command queue family",
                           m_id.GetResource().GetName(),
                           m_change.owner.GetQueueFamilyBefore(),
                           m_change.owner.GetQueueFamilyAfter());
    }
    return "";
}

const ResourceStateChange& ResourceBarrier::GetStateChange() const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(m_id.GetType(), ResourceBarrier::Type::StateTransition);
    return m_change.state;
}

const ResourceOwnerChange& ResourceBarrier::GetOwnerChange() const
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
        META_CHECK_ARG_TRUE_DESCR(m_id.GetResource().GetOwnerQueueFamily().has_value(),
                                  "can not transition resource '{}' ownership which has no existing owner queue family",
                                  m_id.GetResource().GetName());
        META_CHECK_ARG_EQUAL_DESCR(m_id.GetResource().GetOwnerQueueFamily().value(), m_change.owner.GetQueueFamilyBefore(),
                                   "owner of resource '{}' does not match with transition barrier 'before' state",
                                   m_id.GetResource().GetName());
        m_id.GetResource().SetOwnerQueueFamily(m_change.owner.GetQueueFamilyAfter());
        break;
    }
}

Ptr<IResourceBarriers> IResourceBarriers::CreateTransitions(const Refs<IResource>& resources,
                                                            const Opt<ResourceStateChange>& state_change,
                                                            const Opt<ResourceOwnerChange>& owner_change)
{
    META_FUNCTION_TASK();
    Set resource_barriers;
    for (const Ref<IResource>& resource_ref : resources)
    {
        if (owner_change.has_value())
            resource_barriers.emplace(resource_ref.get(), *owner_change);

        if (state_change.has_value())
            resource_barriers.emplace(resource_ref.get(), *state_change);
    }
    return IResourceBarriers::Create(resource_barriers);
}

ResourceBarriersBase::ResourceBarriersBase(const Set& barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::inserter(m_barriers_map, m_barriers_map.begin()),
                   [](const ResourceBarrier& barrier)
                   { return std::pair<ResourceBarrierId, ResourceBarrier>(barrier.GetId(), barrier); });
}

ResourceBarriersBase::Set ResourceBarriersBase::GetSet() const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    Set barriers;
    std::transform(m_barriers_map.begin(), m_barriers_map.end(), std::inserter(barriers, barriers.begin()),
                   [](const auto& barrier_pair) { return barrier_pair.second; });
    return barriers;
}

const ResourceBarrier* ResourceBarriersBase::GetBarrier(const ResourceBarrierId& id) const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(id);
    return barrier_it == m_barriers_map.end() ? nullptr : &barrier_it->second;
}

bool ResourceBarriersBase::HasStateTransition(IResource& resource, ResourceState before, ResourceState after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(ResourceBarrierId(ResourceBarrier::Type::StateTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == ResourceBarrier(resource, before, after);
}

bool ResourceBarriersBase::HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(ResourceBarrierId(ResourceBarrier::Type::OwnerTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == ResourceBarrier(resource, queue_family_before, queue_family_after);
}

ResourceBarriersBase::AddResult ResourceBarriersBase::AddStateTransition(IResource& resource, ResourceState before, ResourceState after)
{
    return Add(ResourceBarrierId(ResourceBarrier::Type::StateTransition, resource), ResourceBarrier(resource, before, after));
}

ResourceBarriersBase::AddResult ResourceBarriersBase::AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
{
    return Add(ResourceBarrierId(ResourceBarrier::Type::OwnerTransition, resource), ResourceBarrier(resource, queue_family_before, queue_family_after));
}

bool ResourceBarriersBase::Remove(ResourceBarrier::Type type, IResource& resource)
{
    return Remove(ResourceBarrierId(type, resource));
}

bool ResourceBarriersBase::RemoveStateTransition(IResource& resource)
{
    return Remove(ResourceBarrierId(ResourceBarrier::Type::StateTransition, resource));
}

bool ResourceBarriersBase::RemoveOwnerTransition(IResource& resource)
{
    return Remove(ResourceBarrierId(ResourceBarrier::Type::OwnerTransition, resource));
}

ResourceBarriersBase::AddResult ResourceBarriersBase::Add(const ResourceBarrierId& id, const ResourceBarrier& barrier)
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

bool ResourceBarriersBase::Remove(const ResourceBarrierId& id)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    return m_barriers_map.erase(id);
}

void ResourceBarriersBase::ApplyTransitions() const
{
    META_FUNCTION_TASK();
    for(const auto& [barrier_id, barrier] : m_barriers_map)
    {
         barrier.ApplyTransition();
    }
}

ResourceBarriersBase::operator std::string() const noexcept
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