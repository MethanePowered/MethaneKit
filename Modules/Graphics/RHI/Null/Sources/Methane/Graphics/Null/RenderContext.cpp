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

FILE: Methane/Graphics/Null/RenderContext.cpp
Null implementation of the render context interface.

******************************************************************************/

#include <Methane/Graphics/Null/RenderContext.h>

namespace Methane::Graphics::Rhi
{

Ptr<IRenderContext> IRenderContext::Create(const Platform::AppEnvironment& env, IDevice& device,
                                           tf::Executor& parallel_executor, const RenderContextSettings& settings)
{
    return std::make_shared<Null::RenderContext>(env, static_cast<Null::Device&>(device), parallel_executor, settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Null
{

RenderContext::RenderContext(const Methane::Platform::AppEnvironment&, Device& device, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
    : Context(device, parallel_executor, settings)
{
}

RenderContext::~RenderContext()
{
    try
    {
        RenderContext::Release();
    }
    catch(const std::exception& e)
    {
        META_UNUSED(e);
        META_LOG("WARNING: Unexpected error during Query destruction: {}", e.what());
        assert(false);
    }
}

void RenderContext::Present()
{
    Context<Base::RenderContext>::Present();
    Context<Base::RenderContext>::OnCpuPresentComplete();
    UpdateFrameBufferIndex();
}

} // namespace Methane::Graphics::Null
