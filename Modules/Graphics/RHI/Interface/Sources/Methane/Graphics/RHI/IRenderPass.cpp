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

FILE: Methane/Graphics/RHI/IRenderPass.cpp
Methane render pass interface: specifies output texture views of the render pattern.

******************************************************************************/

#include <Methane/Graphics/RHI/IRenderPass.h>
#include <Methane/Graphics/RHI/IRenderPattern.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<IRenderPass> IRenderPass::Create(IRenderPattern& render_pattern, const Settings& settings)
{
    META_FUNCTION_TASK();
    return render_pattern.CreateRenderPass(settings);
}

} // namespace Methane::Graphics::Rhi
