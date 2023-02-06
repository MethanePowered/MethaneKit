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

#include <Methane/Graphics/RHI/IResourceBarriers.h>

#include <tracy/Tracy.hpp>

#include <mutex>

namespace Methane::Graphics::Base
{

class ResourceBarriers
    : public Rhi::IResourceBarriers
    , public std::enable_shared_from_this<ResourceBarriers>
{
public:
    explicit ResourceBarriers(const Set& barriers);

    // IResourceBarriers overrides
    [[nodiscard]] Ptr<IResourceBarriers> GetPtr() final     { return shared_from_this(); }
    [[nodiscard]] bool       IsEmpty() const noexcept final { return m_barriers_map.empty(); }
    [[nodiscard]] Set        GetSet() const noexcept final;
    [[nodiscard]] const Map& GetMap() const noexcept final  { return m_barriers_map; }
    [[nodiscard]] explicit operator std::string() const noexcept final;

    [[nodiscard]] const Barrier* GetBarrier(const Barrier::Id& id) const noexcept final;
    [[nodiscard]] bool HasStateTransition(Rhi::IResource& resource, State before, State after) final;
    [[nodiscard]] bool HasOwnerTransition(Rhi::IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) final;

    bool Remove(Rhi::ResourceBarrier::Type type, Rhi::IResource& resource) final;
    bool RemoveStateTransition(Rhi::IResource& resource) final;
    bool RemoveOwnerTransition(Rhi::IResource& resource) final;

    AddResult AddStateTransition(Rhi::IResource& resource, State before, State after) final;
    AddResult AddOwnerTransition(Rhi::IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) final;

    AddResult Add(const Barrier::Id& id, const Barrier& barrier) override;
    bool Remove(const Barrier::Id& id) override;

    void ApplyTransitions() const final;

    auto Lock() const { return std::scoped_lock<LockableBase(std::recursive_mutex)>(m_barriers_mutex); }

private:
    Map m_barriers_map;
    mutable TracyLockable(std::recursive_mutex, m_barriers_mutex);
};

} // namespace Methane::Graphics::Base
