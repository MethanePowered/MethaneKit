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

FILE: Methane/Graphics/RenderStateBase.h
Base implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/IRenderState.h>

#include "ObjectBase.h"

namespace Methane::Graphics
{

class RenderContextBase;
class RenderCommandListBase;

class ViewStateBase
    : public ObjectBase
    , public IViewState
{
public:
    explicit ViewStateBase(const Settings& settings);

    // IViewState overrides
    const Settings& GetSettings() const noexcept override   { return m_settings; }
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // ViewStateBase interface
    virtual void Apply(RenderCommandListBase& command_list) = 0;

private:
    Settings m_settings;
};

class RenderStateBase
    : public ObjectBase
    , public IRenderState
{
public:
    RenderStateBase(const RenderContextBase& context, const Settings& settings);

    // IRenderState overrides
    const Settings& GetSettings() const noexcept override { return m_settings; }
    void Reset(const Settings& settings) override;

    // RenderStateBase interface
    virtual void Apply(RenderCommandListBase& command_list, Groups apply_groups) = 0;

    const RenderContextBase& GetRenderContext() const noexcept { return m_context; }

protected:
    IProgram& GetProgram();

private:
    const RenderContextBase& m_context;
    Settings                 m_settings;
};

} // namespace Methane::Graphics
