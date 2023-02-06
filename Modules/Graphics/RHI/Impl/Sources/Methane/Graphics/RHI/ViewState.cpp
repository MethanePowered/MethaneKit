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

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <ViewState.hh>
#else
#include <ViewState.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ViewState);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(ViewState);

ViewState::ViewState(const Ptr<IViewState>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
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

bool ViewState::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IViewState& ViewState::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IViewState> ViewState::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

const ViewState::Settings& ViewState::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

bool ViewState::Reset(const Settings& settings) const
{
    return GetImpl(m_impl_ptr).Reset(settings);
}

bool ViewState::SetViewports(const Viewports& viewports) const
{
    return GetImpl(m_impl_ptr).SetViewports(viewports);
}

bool ViewState::SetScissorRects(const ScissorRects& scissor_rects) const
{
    return GetImpl(m_impl_ptr).SetScissorRects(scissor_rects);
}

} // namespace Methane::Graphics::Rhi
