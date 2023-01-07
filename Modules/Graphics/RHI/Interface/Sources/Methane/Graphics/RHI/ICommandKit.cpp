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

FILE: Methane/Graphics/RHI/ICommandKit.cpp
Methane command-kit interface.

******************************************************************************/

#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Graphics/RHI/IContext.h>
#include <Methane/Graphics/RHI/ICommandQueue.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandKit> ICommandKit::Create(const IContext& context, CommandListType command_lists_type)
{
    META_FUNCTION_TASK();
    return context.CreateCommandKit(command_lists_type);
}

Ptr<ICommandKit> ICommandKit::Create(ICommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return cmd_queue.CreateCommandKit();
}

} // namespace Methane::Graphics::Rhi
