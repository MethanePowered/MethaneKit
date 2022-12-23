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

FILE: Methane/Graphics/RHI/ResourceBarriers.cpp
Methane ResourceBarriers PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ResourceBarriers.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/RenderPass.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/ResourceBarriers.h>
using ResourceBarriersImpl = Methane::Graphics::DirectX::ResourceBarriers;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/ResourceBarriers.h>
using ResourceBarriersImpl = Methane::Graphics::Vulkan::ResourceBarriers;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Base/ResourceBarriers.h>
using ResourceBarriersImpl = Methane::Graphics::Base::ResourceBarriers;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class ResourceBarriers::Impl
    : public ImplWrapper<IResourceBarriers, ResourceBarriersImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ResourceBarriers);

ResourceBarriers::ResourceBarriers(const Ptr<IResourceBarriers>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

ResourceBarriers::ResourceBarriers(IResourceBarriers& interface_ref)
    : ResourceBarriers(std::dynamic_pointer_cast<IResourceBarriers>(interface_ref.GetPtr()))
{
}

ResourceBarriers::ResourceBarriers(const Set& barriers)
    : ResourceBarriers(IResourceBarriers::Create(barriers))
{
}

ResourceBarriers::ResourceBarriers(const Refs<IResource>& resources,
                                   const Opt<Barrier::StateChange>& state_change,
                                   const Opt<Barrier::OwnerChange>& owner_change)
    : ResourceBarriers(IResourceBarriers::CreateTransitions(resources, state_change, owner_change))
{
}

void ResourceBarriers::Init(const Set& barriers)
{
    m_impl_ptr = std::make_unique<Impl>(IResourceBarriers::Create(barriers));
}

void ResourceBarriers::Init(const Refs<IResource>& resources,
                            const Opt<Barrier::StateChange>& state_change,
                            const Opt<Barrier::OwnerChange>& owner_change)
{
    m_impl_ptr = std::make_unique<Impl>(IResourceBarriers::CreateTransitions(resources, state_change, owner_change));
}

void ResourceBarriers::Release()
{
    m_impl_ptr.release();
}

bool ResourceBarriers::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IResourceBarriers& ResourceBarriers::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IResourceBarriers> ResourceBarriers::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

bool ResourceBarriers::IsEmpty() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).IsEmpty();
}

ResourceBarriers::Set ResourceBarriers::GetSet() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSet();
}

const ResourceBarriers::Map& ResourceBarriers::GetMap() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetMap();
}

const ResourceBarriers::Barrier* ResourceBarriers::GetBarrier(const Barrier::Id& id) const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetBarrier(id);
}

bool ResourceBarriers::HasStateTransition(IResource& resource, State before, State after) const
{
    return GetPrivateImpl(m_impl_ptr).HasStateTransition(resource, before, after);
}

bool ResourceBarriers::HasOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) const
{
    return GetPrivateImpl(m_impl_ptr).HasOwnerTransition(resource, queue_family_before, queue_family_after);
}

ResourceBarriers::operator std::string() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).operator std::string();
}

bool ResourceBarriers::Remove(Barrier::Type type, IResource& resource) const
{
    return Remove(Barrier::Id(type, resource));
}

bool ResourceBarriers::RemoveStateTransition(IResource& resource) const
{
    return GetPrivateImpl(m_impl_ptr).RemoveStateTransition(resource);
}

bool ResourceBarriers::RemoveOwnerTransition(IResource& resource) const
{
    return GetPrivateImpl(m_impl_ptr).RemoveOwnerTransition(resource);
}

ResourceBarriers::AddResult ResourceBarriers::AddStateTransition(IResource& resource, State before, State after) const
{
    return GetPrivateImpl(m_impl_ptr).AddStateTransition(resource, before, after);
}

ResourceBarriers::AddResult ResourceBarriers::AddOwnerTransition(IResource& resource, uint32_t queue_family_before, uint32_t queue_family_after) const
{
    return GetPrivateImpl(m_impl_ptr).AddOwnerTransition(resource, queue_family_before, queue_family_after);
}

ResourceBarriers::AddResult ResourceBarriers::Add(const Barrier::Id& id, const Barrier& barrier) const
{
    return GetPrivateImpl(m_impl_ptr).Add(id, barrier);
}

bool ResourceBarriers::Remove(const Barrier::Id& id) const
{
    return GetPrivateImpl(m_impl_ptr).Remove(id);
}

void ResourceBarriers::ApplyTransitions() const
{
    return GetPrivateImpl(m_impl_ptr).ApplyTransitions();
}

} // namespace Methane::Graphics::Rhi
