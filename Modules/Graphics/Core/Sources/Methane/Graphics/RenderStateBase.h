/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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

#include <Methane/Graphics/RenderState.h>

#include "ObjectBase.h"

namespace Methane::Graphics
{

class RenderContextBase;
class RenderCommandListBase;

class RenderStateBase
    : public ObjectBase
    , public RenderState
    , public std::enable_shared_from_this<RenderStateBase>
{
public:
    RenderStateBase(RenderContextBase& context, const Settings& settings);

    // RenderState interface
    const Settings& GetSettings() const override   { return m_settings; }
    void Reset(const Settings& settings) override;
    void SetViewports(const Viewports& viewports) override;
    void SetScissorRects(const ScissorRects& scissor_rects) override;

    // RenderStateBase interface
    virtual void Apply(RenderCommandListBase& command_list, Group::Mask apply_groups) = 0;

    Ptr<RenderStateBase> GetPtr()           { return shared_from_this(); }
    RenderContextBase&   GetRenderContext() { return m_context; }

protected:
    Program& GetProgram();

private:
    RenderContextBase& m_context;
    Settings           m_settings;
};

} // namespace Methane::Graphics
