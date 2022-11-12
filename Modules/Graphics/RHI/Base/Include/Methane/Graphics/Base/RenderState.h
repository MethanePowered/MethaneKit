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

FILE: Methane/Graphics/Base/RenderState.h
Base implementation of the render state interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/IRenderState.h>

namespace Methane::Graphics::Base
{

class RenderContext;
class RenderCommandList;

class ViewState
    : public Object
    , public IViewState
{
public:
    explicit ViewState(const Settings& settings);

    // IViewState overrides
    const Settings& GetSettings() const noexcept override   { return m_settings; }
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // ViewState interface
    virtual void Apply(RenderCommandList& command_list) = 0;

private:
    Settings m_settings;
};

class RenderState
    : public Object
    , public IRenderState
{
public:
    RenderState(const RenderContext& context, const Settings& settings);

    // IRenderState overrides
    const Settings& GetSettings() const noexcept override { return m_settings; }
    void Reset(const Settings& settings) override;

    // RenderState interface
    virtual void Apply(RenderCommandList& command_list, Groups apply_groups) = 0;

    const RenderContext& GetRenderContext() const noexcept { return m_context; }

protected:
    IProgram& GetProgram();

private:
    const RenderContext& m_context;
    Settings             m_settings;
};

} // namespace Methane::Graphics::Base
