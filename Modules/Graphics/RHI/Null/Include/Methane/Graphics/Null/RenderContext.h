/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/RenderContext.hh
Null implementation of the render context interface.

******************************************************************************/

#pragma once

#include "Context.hpp"
#include "Device.h"

#include <Methane/Graphics/Base/RenderContext.h>
#include <Methane/Platform/AppEnvironment.h>

namespace Methane::Graphics::Null
{

class RenderContext final // NOSONAR - this class requires destructor
    : public Context<Base::RenderContext>
{
public:
    RenderContext(const Platform::AppEnvironment& app_env, Device& device,
                  tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings);
    ~RenderContext() override;

    // IRenderContext interface
    [[nodiscard]] Ptr<Rhi::IRenderState> CreateRenderState(const Rhi::RenderStateSettings& settings) const override;
    [[nodiscard]] Ptr<Rhi::IRenderPattern> CreateRenderPattern(const Rhi::RenderPatternSettings& settings) override;
    bool     ReadyToRender() const override { return true; }
    void     Present() override;
    Platform::AppView GetAppView() const override { return { }; }
};

} // namespace Methane::Graphics::Null
