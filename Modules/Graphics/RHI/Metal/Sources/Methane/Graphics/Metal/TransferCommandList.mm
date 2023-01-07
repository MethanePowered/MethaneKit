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

FILE: Methane/Graphics/Metal/TransferCommandList.mm
Metal implementation of the transfer command list interface.

******************************************************************************/

#include <Methane/Graphics/Metal/TransferCommandList.hh>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Metal
{

TransferCommandList::TransferCommandList(Base::CommandQueue& command_queue)
    : CommandList<id<MTLBlitCommandEncoder>, Base::CommandList>(true, command_queue, Rhi::CommandListType::Transfer)
{
    META_FUNCTION_TASK();
}

void TransferCommandList::Reset(Rhi::ICommandListDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    if (IsCommandEncoderInitialized())
    {
        Base::CommandList::Reset(debug_group_ptr);
        return;
    }

    const id<MTLCommandBuffer>& mtl_cmd_buffer = InitializeCommandBuffer();
    InitializeCommandEncoder([mtl_cmd_buffer blitCommandEncoder]);
    Base::CommandList::Reset(debug_group_ptr);
}

} // namespace Methane::Graphics::Metal
