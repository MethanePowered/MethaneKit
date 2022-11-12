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

FILE: Methane/Graphics/Base/ResourceBarriers.h
Methane resource barriers base implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/IResourceBarriers.h>

#include <Tracy.hpp>

#include <mutex>

namespace Methane::Graphics::Base
{

class ResourceBarriers
    : public IResourceBarriers
{
public:
    explicit ResourceBarriers(const Set& barriers);

    // IResourceBarriers overrides
    [[nodiscard]] bool       IsEmpty() const noexcept override { return m_barriers_map.empty(); }
    [[nodiscard]] Set        GetSet() const noexcept override;
    [[nodiscard]] const Map& GetMap() const noexcept override { return m_barriers_map; }
    [[nodiscard]] explicit operator std::string() const noexcept override;

    [[nodiscard]] const ResourceBarrier* GetBarrier(const ResourceBarrier::Id& id) const noexcept override;
    [[nodiscard]] bool HasStateTransition(IResource& resource, ResourceState before, ResourceState after) override;
    [[nodiscard]] bool HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) override;

    bool Remove(ResourceBarrier::Type type, IResource& resource) override;
    bool RemoveStateTransition(IResource& resource) override;
    bool RemoveOwnerTransition(IResource& resource) override;

    AddResult AddStateTransition(IResource& resource, ResourceState before, ResourceState after) override;
    AddResult AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) override;

    AddResult Add(const ResourceBarrier::Id& id, const ResourceBarrier& barrier) override;
    bool Remove(const ResourceBarrier::Id& id) override;

    void ApplyTransitions() const override;

    auto Lock() const { return std::scoped_lock<LockableBase(std::recursive_mutex)>(m_barriers_mutex); }

private:
    Map m_barriers_map;
    mutable TracyLockable(std::recursive_mutex, m_barriers_mutex);
};

} // namespace Methane::Graphics::Base
