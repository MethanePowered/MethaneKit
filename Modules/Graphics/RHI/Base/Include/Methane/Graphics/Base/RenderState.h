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

#include <Methane/Graphics/RHI/IRenderState.h>

namespace Methane::Graphics::Base
{

class RenderContext;
class RenderCommandList;

class RenderState
    : public Object
    , public Rhi::IRenderState
{
public:
    RenderState(const RenderContext& context, const Settings& settings, bool is_deferred = false);

    // IRenderState overrides
    const Settings& GetSettings() const noexcept override { return m_settings; }
    void Reset(const Settings& settings) override;

    // RenderState interface
    virtual void Apply(RenderCommandList& command_list, Groups apply_groups) = 0;

    const RenderContext& GetRenderContext() const noexcept { return m_context; }
    bool                 IsDeferred() const noexcept       { return m_is_deferred; }

protected:
    Rhi::IProgram& GetProgram();

private:
    const RenderContext& m_context;
    Settings             m_settings;
    
    // Deferred state is applied on first Draw, instead of SetRenderState call
    // This is required for Vulkan without dynamic state support (on mobile platforms):
    // Vulkan monolithic pipeline state is created by settings of RenderState, ViewState and PrimitiveType
    // and is applied on first Draw call when whole state is fully defined
    const bool           m_is_deferred;
};

} // namespace Methane::Graphics::Base
