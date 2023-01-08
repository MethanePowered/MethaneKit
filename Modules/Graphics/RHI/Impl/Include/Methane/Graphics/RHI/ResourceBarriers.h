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

namespace Methane::Graphics::META_GFX_NAME
{
class ResourceBarriers;
}

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
    META_PIMPL_METHODS_COMPARE_DECLARE(ResourceBarriers);

    META_RHI_API explicit ResourceBarriers(const Ptr<IResourceBarriers>& interface_ptr);
    META_RHI_API explicit ResourceBarriers(IResourceBarriers& interface_ref);
    META_RHI_API explicit ResourceBarriers(const Set& barriers);
    META_RHI_API ResourceBarriers(const Refs<IResource>& resources,
                                  const Opt<Barrier::StateChange>& state_change,
                                  const Opt<Barrier::OwnerChange>& owner_change);

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IResourceBarriers& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IResourceBarriers> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IResourceBarriers interface methods
    [[nodiscard]] META_RHI_API bool  IsEmpty() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Set   GetSet() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Map& GetMap() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const Barrier* GetBarrier(const Barrier::Id& id) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API bool  HasStateTransition(IResource& resource, State before, State after) const;
    [[nodiscard]] META_RHI_API bool  HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) const;
    [[nodiscard]] META_RHI_API explicit operator std::string() const META_PIMPL_NOEXCEPT;

    META_RHI_API bool Remove(Barrier::Type type, IResource& resource) const;
    META_RHI_API bool RemoveStateTransition(IResource& resource) const;
    META_RHI_API bool RemoveOwnerTransition(IResource& resource) const;

    META_RHI_API AddResult AddStateTransition(IResource& resource, State before, State after) const;
    META_RHI_API AddResult AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) const;

    META_RHI_API AddResult Add(const Barrier::Id& id, const Barrier& barrier) const;
    META_RHI_API bool      Remove(const Barrier::Id& id) const;

    META_RHI_API void ApplyTransitions() const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ResourceBarriers;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/ResourceBarriers.cpp>

#endif // META_RHI_PIMPL_INLINE
