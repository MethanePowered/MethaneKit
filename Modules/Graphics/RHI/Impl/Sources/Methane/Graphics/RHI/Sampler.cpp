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

FILE: Methane/Graphics/RHI/Sampler.cpp
Methane Sampler PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/ResourceBarriers.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <Sampler.hh>
#else
#include <Sampler.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Sampler);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(Sampler);

Sampler::Sampler(const Ptr<ISampler>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Sampler::Sampler(ISampler& interface_ref)
    : Sampler(interface_ref.GetDerivedPtr<ISampler>())
{
}

Sampler::Sampler(const RenderContext& context, const Settings& settings)
    : Sampler(ISampler::Create(context.GetInterface(), settings))
{
}

Sampler::Sampler(const ComputeContext& context, const Settings& settings)
    : Sampler(ISampler::Create(context.GetInterface(), settings))
{
}

bool Sampler::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

ISampler& Sampler::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<ISampler> Sampler::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Sampler::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Sampler::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void Sampler::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void Sampler::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

bool Sampler::SetState(State state) const
{
    return GetImpl(m_impl_ptr).SetState(state);
}

bool Sampler::SetState(State state, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterfacePtr();
    const bool state_changed = GetImpl(m_impl_ptr).SetState(state, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

bool Sampler::SetOwnerQueueFamily(uint32_t family_index) const
{
    return GetImpl(m_impl_ptr).SetOwnerQueueFamily(family_index);
}

bool Sampler::SetOwnerQueueFamily(uint32_t family_index, Barriers& out_barriers) const
{
    Ptr<IResourceBarriers> out_barriers_ptr = out_barriers.GetInterfacePtr();
    const bool             state_changed    = GetImpl(m_impl_ptr).SetOwnerQueueFamily(family_index, out_barriers_ptr);
    if (!out_barriers.IsInitialized() && out_barriers_ptr)
    {
        out_barriers = ResourceBarriers(out_barriers_ptr);
    }
    return state_changed;
}

void Sampler::RestoreDescriptorViews(const DescriptorByViewId& descriptor_by_view_id) const
{
    GetImpl(m_impl_ptr).RestoreDescriptorViews(descriptor_by_view_id);
}

ResourceType Sampler::GetResourceType() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetResourceType();
}

ResourceState Sampler::GetState() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetState();
}

ResourceUsageMask Sampler::GetUsage() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetUsage();
}

const Sampler::DescriptorByViewId& Sampler::GetDescriptorByViewId() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetDescriptorByViewId();
}

const IContext& Sampler::GetContext() const
{
    return GetImpl(m_impl_ptr).GetContext();
}

const Opt<uint32_t>& Sampler::GetOwnerQueueFamily() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetOwnerQueueFamily();
}

void Sampler::Connect(Data::Receiver<IResourceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IResourceCallback>::Connect(receiver);
}

void Sampler::Disconnect(Data::Receiver<IResourceCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IResourceCallback>::Disconnect(receiver);
}

const Sampler::Settings& Sampler::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
