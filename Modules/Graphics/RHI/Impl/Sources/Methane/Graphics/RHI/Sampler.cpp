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

#if defined METHANE_GFX_METAL
#include <Sampler.hh>
#else
#include <Sampler.h>
#endif

#include "Pimpl.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Sampler);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(Sampler);

Sampler::Sampler(Ptr<Impl>&& impl_ptr)
    : Transmitter<IObjectCallback>(*impl_ptr)
    , Transmitter<IResourceCallback>(*impl_ptr)
    , m_impl_ptr(std::move(impl_ptr))
{
}

Sampler::Sampler(const Ptr<ISampler>& interface_ptr)
    : Sampler(std::dynamic_pointer_cast<Impl>(interface_ptr))
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

void Sampler::Init(const RenderContext& context, const Settings& settings)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(ISampler::Create(context.GetInterface(), settings));
    Transmitter<IObjectCallback>::Reset(m_impl_ptr.get());
    Transmitter<IResourceCallback>::Reset(m_impl_ptr.get());
}

void Sampler::Release()
{
    Transmitter<IObjectCallback>::Reset();
    Transmitter<IResourceCallback>::Reset();
    m_impl_ptr.reset();
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

const Sampler::Settings& Sampler::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
