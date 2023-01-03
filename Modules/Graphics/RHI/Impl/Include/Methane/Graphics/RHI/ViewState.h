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

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IViewState.h>

namespace Methane::Graphics::META_GFX_NAME
{
class ViewState;
}

namespace Methane::Graphics::Rhi
{

class ViewState
{
public:
    using Settings = ViewSettings;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ViewState);
    META_PIMPL_METHODS_COMPARE_DECLARE(ViewState);

    META_RHI_API explicit ViewState(const Ptr<IViewState>& interface_ptr);
    META_RHI_API explicit ViewState(IViewState& interface_ref);
    META_RHI_API explicit ViewState(const Settings& settings);

    META_RHI_API void Init(const Settings& settings);
    META_RHI_API void Release();

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IViewState& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IViewState> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IViewState interface methods
    [[nodiscard]] META_RHI_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    META_RHI_API bool Reset(const Settings& settings) const;
    META_RHI_API bool SetViewports(const Viewports& viewports) const;
    META_RHI_API bool SetScissorRects(const ScissorRects& scissor_rects) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ViewState;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/ViewState.cpp>

#endif // META_RHI_PIMPL_INLINE