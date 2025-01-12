/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IResourceBarriers.cpp
Methane resource barriers for manual or automatic resource state synchronization on GPU.

******************************************************************************/

#include <Methane/Graphics/RHI/IResourceBarriers.h>
#include <Methane/Graphics/RHI/IResource.h>

#include <magic_enum/magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

ResourceBarrierId::ResourceBarrierId(Type type, Rhi::IResource& resource) noexcept
    : m_type(type)
    , m_resource_ref(resource)
{ }

ResourceStateChange::ResourceStateChange(ResourceState before, ResourceState after) noexcept
    : m_before(before)
    , m_after(after)
{ }

ResourceOwnerChange::ResourceOwnerChange(uint32_t queue_family_before, uint32_t queue_family_after) noexcept
    : m_queue_family_before(queue_family_before)
    , m_queue_family_after(queue_family_after)
{ }

ResourceBarrier::ResourceBarrier(IResource& resource, const StateChange& state_change)
    : m_id(Type::StateTransition, resource)
    , m_change(state_change)
{ }

ResourceBarrier::ResourceBarrier(IResource& resource, const OwnerChange& owner_change)
    : m_id(Type::OwnerTransition, resource)
    , m_change(owner_change)
{ }

ResourceBarrier::ResourceBarrier(IResource& resource, ResourceState state_before, ResourceState state_after)
    : ResourceBarrier(resource, StateChange(state_before, state_after))
{ }

ResourceBarrier::ResourceBarrier(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after)
    : ResourceBarrier(resource, OwnerChange(queue_family_before, queue_family_after))
{ }

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
    META_CHECK_EQUAL(m_id.GetType(), ResourceBarrier::Type::StateTransition);
    return m_change.state;
}

const ResourceOwnerChange& ResourceBarrier::GetOwnerChange() const
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(m_id.GetType(), ResourceBarrier::Type::OwnerTransition);
    return m_change.owner;
}

void ResourceBarrier::ApplyTransition() const
{
    META_FUNCTION_TASK();
    switch(m_id.GetType())
    {
    case Type::StateTransition:
        META_CHECK_EQUAL_DESCR(m_id.GetResource().GetState(), m_change.state.GetStateBefore(),
                               "state of resource '{}' does not match with transition barrier 'before' state",
                               m_id.GetResource().GetName());
        m_id.GetResource().SetState(m_change.state.GetStateAfter());
        break;

    case Type::OwnerTransition:
        META_CHECK_TRUE_DESCR(m_id.GetResource().GetOwnerQueueFamily().has_value(),
                              "can not transition resource '{}' ownership which has no existing owner queue family",
                              m_id.GetResource().GetName());
        META_CHECK_EQUAL_DESCR(m_id.GetResource().GetOwnerQueueFamily().value(), m_change.owner.GetQueueFamilyBefore(),
                               "owner of resource '{}' does not match with transition barrier 'before' state",
                               m_id.GetResource().GetName());
        m_id.GetResource().SetOwnerQueueFamily(m_change.owner.GetQueueFamilyAfter());
        break;
    }
}

Ptr<IResourceBarriers> IResourceBarriers::CreateTransitions(RefSpan<IResource> resources,
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

Ptr<IResourceBarriers> IResourceBarriers::CreateTransitions(const Refs<IResource>& resources,
                                                            const Opt<ResourceStateChange>& state_change,
                                                            const Opt<ResourceOwnerChange>& owner_change)
{
    return CreateTransitions(RefSpan<IResource>(resources), state_change, owner_change);
}

} // namespace Methane::Graphics::Rhi
