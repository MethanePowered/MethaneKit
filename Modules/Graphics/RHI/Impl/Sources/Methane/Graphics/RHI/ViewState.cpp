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

FILE: Methane/Graphics/RHI/ViewState.cpp
Methane ViewState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ViewState.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/ViewState.h>
using ViewStateImpl = Methane::Graphics::DirectX::ViewState;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/ViewState.h>
using ViewStateImpl = Methane::Graphics::Vulkan::ViewState;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/ViewState.hh>
using ViewStateImpl = Methane::Graphics::Metal::ViewState;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class ViewState::Impl
    : public ImplWrapper<IViewState, ViewStateImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ViewState);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(ViewState);

ViewState::ViewState(const Ptr<IViewState>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

ViewState::ViewState(IViewState& interface_ref)
    : ViewState(std::dynamic_pointer_cast<IViewState>(interface_ref.GetPtr()))
{
}

ViewState::ViewState(const Settings& settings)
    : ViewState(IViewState::Create(settings))
{
}

void ViewState::Init(const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(IViewState::Create(settings));
}

void ViewState::Release()
{
    m_impl_ptr.reset();
}

bool ViewState::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IViewState& ViewState::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IViewState> ViewState::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

const ViewState::Settings& ViewState::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

bool ViewState::Reset(const Settings& settings) const
{
    return GetPrivateImpl(m_impl_ptr).Reset(settings);
}

bool ViewState::SetViewports(const Viewports& viewports) const
{
    return GetPrivateImpl(m_impl_ptr).SetViewports(viewports);
}

bool ViewState::SetScissorRects(const ScissorRects& scissor_rects) const
{
    return GetPrivateImpl(m_impl_ptr).SetScissorRects(scissor_rects);
}

} // namespace Methane::Graphics::Rhi
