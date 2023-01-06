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

FILE: Methane/Graphics/Null/RenderPass.cpp
Null implementation of the render pass interface.

******************************************************************************/

#include <Methane/Graphics/Null/RenderPass.h>
#include <Methane/Graphics/Null/RenderPattern.h>

namespace Methane::Graphics::Rhi
{

Ptr<IRenderPass> IRenderPass::Create(IRenderPattern& render_pattern, const Settings& settings)
{
    return std::make_shared<Null::RenderPass>(dynamic_cast<Null::RenderPattern&>(render_pattern), settings);
}

} // namespace Methane::Graphics::Rhi
