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

FILE: Methane/Graphics/Null/ParallelRenderCommandList.cpp
Null implementation of the render command list interface.

******************************************************************************/

#include <Methane/Graphics/Null/ParallelRenderCommandList.h>
#include <Methane/Graphics/Null/RenderPass.h>
#include <Methane/Graphics/Null/CommandQueue.h>

namespace Methane::Graphics::Rhi
{

Ptr<IParallelRenderCommandList> IParallelRenderCommandList::Create(ICommandQueue& command_queue, IRenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<Null::ParallelRenderCommandList>(static_cast<Null::CommandQueue&>(command_queue),
                                                             static_cast<Null::RenderPass&>(render_pass));
}

} // namespace Methane::Graphics::Rhi
