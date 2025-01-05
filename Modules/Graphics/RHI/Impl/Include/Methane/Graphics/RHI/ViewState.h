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

FILE: Methane/Graphics/RHI/ViewState.h
Methane ViewState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IViewState.h>

namespace Methane::Graphics::META_GFX_NAME
{
class ViewState;
}

namespace Methane::Graphics::Rhi
{

class ViewState // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Settings = ViewSettings;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ViewState);
    META_PIMPL_METHODS_COMPARE_INLINE(ViewState);

    META_PIMPL_API explicit ViewState(const Ptr<IViewState>& interface_ptr);
    META_PIMPL_API explicit ViewState(IViewState& interface_ref);
    META_PIMPL_API explicit ViewState(const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IViewState& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IViewState> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IViewState interface methods
    [[nodiscard]] META_PIMPL_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API bool Reset(const Settings& settings) const;
    META_PIMPL_API bool SetViewports(const Viewports& viewports) const;
    META_PIMPL_API bool SetScissorRects(const ScissorRects& scissor_rects) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ViewState;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/ViewState.cpp>

#endif // META_PIMPL_INLINE
