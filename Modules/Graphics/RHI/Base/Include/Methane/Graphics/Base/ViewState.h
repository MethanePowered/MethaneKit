/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ViewState.h
Base implementation of the view state interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/IViewState.h>
#include <Methane/Data/Emitter.hpp>

namespace Methane::Graphics::Base
{

class RenderCommandList;

class ViewState // NOSONAR - class requires destructor
    : public Rhi::IViewState
    , public Data::Emitter<Rhi::IViewStateCallback>
    , public std::enable_shared_from_this<ViewState>
{
public:
    explicit ViewState(const Settings& settings);
    ~ViewState() override;

    ViewState(const ViewState&) = delete;
    ViewState(ViewState&&) noexcept = delete;

    ViewState& operator=(const ViewState&) = delete;
    ViewState& operator=(ViewState&&) noexcept = delete;

    // IViewState overrides
    Ptr<Rhi::IViewState> GetPtr() final                  { return shared_from_this(); }
    const Settings& GetSettings() const noexcept final   { return m_settings; }
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // ViewState interface
    virtual void Apply(RenderCommandList& command_list) = 0;

private:
    Settings m_settings;
};

} // namespace Methane::Graphics::Base
