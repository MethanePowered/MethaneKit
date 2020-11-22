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

FILE: Methane/Graphics/Vulkan/RenderStateVK.h
Vulkan implementation of the render state interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RenderStateBase.h>

#include <vector>

namespace Methane::Graphics
{

struct IContextVK;

class ViewStateVK final : public ViewStateBase
{
public:
    explicit ViewStateVK(const Settings& settings);

    // ViewState overrides
    bool Reset(const Settings& settings) override;
    bool SetViewports(const Viewports& viewports) override;
    bool SetScissorRects(const ScissorRects& scissor_rects) override;

    // ViewStateBase interface
    void Apply(RenderCommandListBase& command_list) override;
};

class RenderStateVK final : public RenderStateBase
{
public:
    RenderStateVK(RenderContextBase& context, const Settings& settings);
    ~RenderStateVK() override;
    
    // RenderState interface
    void Reset(const Settings& settings) override;

    // RenderStateBase interface
    void Apply(RenderCommandListBase& command_list, Groups state_groups) override;

    // Object interface
    void SetName(const std::string& name) override;

private:
    IContextVK& GetContextVK() noexcept;
    
    void ResetNativeState();
};

} // namespace Methane::Graphics
