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

FILE: Methane/Graphics/RHI/RenderState.cpp
Methane RenderState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/RenderState.h>
using RenderStateImpl = Methane::Graphics::DirectX::RenderState;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/RenderState.h>
using RenderStateImpl = Methane::Graphics::Vulkan::RenderState;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/RenderState.hh>
using RenderStateImpl = Methane::Graphics::Metal::RenderState;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class RenderState::Impl
    : public ImplWrapper<IRenderState, RenderStateImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderState);

RenderState::RenderState(UniquePtr<Impl>&& impl_ptr)
    : Transmitter(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

RenderState::RenderState(const Ptr<IRenderState>& interface_ptr)
    : RenderState(std::make_unique<Impl>(interface_ptr))
{
}

RenderState::RenderState(IRenderState& interface_ref)
    : RenderState(std::dynamic_pointer_cast<IRenderState>(interface_ref.GetPtr()))
{
}

RenderState::RenderState(const RenderContext& context, const Settings& settings)
    : RenderState(IRenderState::Create(context.GetInterface(), settings))
{
}

void RenderState::Init(const RenderContext& context, const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(IRenderState::Create(context.GetInterface(), settings));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void RenderState::Release()
{
    Transmitter::Reset();
    m_impl_ptr.release();
}

bool RenderState::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderState& RenderState::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

const RenderState::Settings& RenderState::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

void RenderState::Reset(const Settings& settings) const
{
    return GetPrivateImpl(m_impl_ptr).Reset(settings);
}

} // namespace Methane::Graphics::Rhi
