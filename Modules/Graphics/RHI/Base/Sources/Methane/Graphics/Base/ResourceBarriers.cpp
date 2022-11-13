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

#include <Methane/Graphics/RHI/IResource.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::Base
{

ResourceBarriers::ResourceBarriers(const Set& barriers)
{
    META_FUNCTION_TASK();
    std::transform(barriers.begin(), barriers.end(), std::inserter(m_barriers_map, m_barriers_map.begin()),
                   [](const Barrier& barrier)
                   { return std::pair<Barrier::Id, Barrier>(barrier.GetId(), barrier); });
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

const Rhi::ResourceBarrier* ResourceBarriers::GetBarrier(const Barrier::Id& id) const noexcept
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(id);
    return barrier_it == m_barriers_map.end() ? nullptr : &barrier_it->second;
}

bool ResourceBarriers::HasStateTransition(Rhi::IResource& resource, State before, State after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(Barrier::Id(Barrier::Type::StateTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == Barrier(resource, before, after);
}

bool ResourceBarriers::HasOwnerTransition(Rhi::IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_barriers_mutex);
    const auto barrier_it = m_barriers_map.find(Barrier::Id(Barrier::Type::OwnerTransition, resource));
    return barrier_it != m_barriers_map.end() &&
           barrier_it->second == Barrier(resource, queue_family_before, queue_family_after);
}

ResourceBarriers::AddResult ResourceBarriers::AddStateTransition(Rhi::IResource& resource, State before, State after)
{
    return Add(Barrier::Id(Barrier::Type::StateTransition, resource), Barrier(resource, before, after));
}

ResourceBarriers::AddResult ResourceBarriers::AddOwnerTransition(Rhi::IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
{
    return Add(Barrier::Id(Barrier::Type::OwnerTransition, resource), Barrier(resource, queue_family_before, queue_family_after));
}

bool ResourceBarriers::Remove(Barrier::Type type, Rhi::IResource& resource)
{
    return Remove(Barrier::Id(type, resource));
}

bool ResourceBarriers::RemoveStateTransition(Rhi::IResource& resource)
{
    return Remove(Barrier::Id(Barrier::Type::StateTransition, resource));
}

bool ResourceBarriers::RemoveOwnerTransition(Rhi::IResource& resource)
{
    return Remove(Barrier::Id(Barrier::Type::OwnerTransition, resource));
}

ResourceBarriers::AddResult ResourceBarriers::Add(const Barrier::Id& id, const Barrier& barrier)
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

bool ResourceBarriers::Remove(const Barrier::Id& id)
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