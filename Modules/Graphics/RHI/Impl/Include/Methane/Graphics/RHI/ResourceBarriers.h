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

FILE: Methane/Graphics/RHI/ResourceBarriers.h
Methane ResourceBarriers PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IResourceBarriers.h>

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderPass;

class ResourceBarriers
{
public:
    using State     = IResourceBarriers::State;
    using Barrier   = IResourceBarriers::Barrier;
    using Map       = IResourceBarriers::Map;
    using Set       = IResourceBarriers::Set;
    using AddResult = IResourceBarriers::AddResult;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ResourceBarriers);

    ResourceBarriers(const Ptr<IResourceBarriers>& interface_ptr);
    ResourceBarriers(IResourceBarriers& interface_ref);
    ResourceBarriers(const Set& barriers);
    ResourceBarriers(const Refs<IResource>& resources,
                     const Opt<Barrier::StateChange>& state_change,
                     const Opt<Barrier::OwnerChange>& owner_change);

    void Init(const Set& barriers);
    void Init(const Refs<IResource>& resources,
              const Opt<Barrier::StateChange>& state_change,
              const Opt<Barrier::OwnerChange>& owner_change);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IResourceBarriers& GetInterface() const META_PIMPL_NOEXCEPT;

    [[nodiscard]] bool  IsEmpty() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Set   GetSet() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Map& GetMap() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Barrier* GetBarrier(const Barrier::Id& id) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] bool  HasStateTransition(IResource& resource, State before, State after) const;
    [[nodiscard]] bool  HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) const;
    [[nodiscard]] explicit operator std::string() const META_PIMPL_NOEXCEPT;

    bool Remove(Barrier::Type type, IResource& resource) const;
    bool RemoveStateTransition(IResource& resource) const;
    bool RemoveOwnerTransition(IResource& resource) const;

    AddResult AddStateTransition(IResource& resource, State before, State after) const;
    AddResult AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) const;

    AddResult Add(const Barrier::Id& id, const Barrier& barrier) const;
    bool      Remove(const Barrier::Id& id) const;

    void ApplyTransitions() const;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
