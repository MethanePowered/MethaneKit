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

namespace Methane::Graphics::Rhi
{

class ViewState
{
public:
    using Settings = ViewSettings;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ViewState);

    ViewState(const Ptr<IViewState>& interface_ptr);
    ViewState(IViewState& interface_ref);
    ViewState(const Settings& settings);

    void Init(const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IViewState& GetInterface() const META_PIMPL_NOEXCEPT;

    // IViewState interface methods
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    bool Reset(const Settings& settings) const;
    bool SetViewports(const Viewports& viewports) const;
    bool SetScissorRects(const ScissorRects& scissor_rects) const;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
