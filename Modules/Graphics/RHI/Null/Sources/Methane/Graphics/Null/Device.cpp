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

FILE: Methane/Graphics/Null/Buffer.cpp
Null implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/Null/Device.h>
#include <Methane/Graphics/Null/RenderContext.h>
#include <Methane/Graphics/Null/ComputeContext.h>

namespace Methane::Graphics::Null
{

Ptr<Rhi::IRenderContext> Device::CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
{
    auto render_context_ptr = std::make_shared<RenderContext>(env, *this, parallel_executor, settings);
    render_context_ptr->Initialize(*this, true);
    return render_context_ptr;
}

[[nodiscard]] Ptr<Rhi::IComputeContext> Device::CreateComputeContext(tf::Executor& parallel_executor, const Rhi::ComputeContextSettings& settings)
{
    auto compute_context_ptr = std::make_shared<ComputeContext>(*this, parallel_executor, settings);
    compute_context_ptr->Initialize(*this, true);
    return compute_context_ptr;
}

} // namespace Methane::Graphics::Null
