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

FILE: Methane/Graphics/Metal/ComputeCommandList.mm
Metal implementation of the compute command list interface.

******************************************************************************/

#include <Methane/Graphics/Metal/ComputeCommandList.hh>
#include <Methane/Graphics/Metal/ComputeState.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

ComputeCommandList::ComputeCommandList(Base::CommandQueue& command_queue)
    : CommandList(true, command_queue)
{ }

void ComputeCommandList::Reset(Rhi::ICommandListDebugGroup* debug_group_ptr)
{
    META_FUNCTION_TASK();
    if (IsCommandEncoderInitialized())
    {
        Base::CommandList::Reset(debug_group_ptr);
        return;
    }

    InitializeCommandBufferAndEncoder([](id<MTLCommandBuffer> mtl_cmd_buffer)
                                      { return [mtl_cmd_buffer computeCommandEncoder]; });
    
    Base::CommandList::Reset(debug_group_ptr);
}

void ComputeCommandList::Dispatch(const Rhi::ThreadGroupsCount& thread_groups_count)
{
    META_FUNCTION_TASK();
    Base::ComputeCommandList::Dispatch(thread_groups_count);

    const auto& mtl_cmd_encoder = GetNativeCommandEncoder();
    META_CHECK_NOT_NULL(mtl_cmd_encoder);

    const Rhi::ThreadGroupSize& thread_group_size = GetComputeState().GetSettings().thread_group_size;
    const MTLSize mtl_thread_groups{ thread_groups_count.GetWidth(), thread_groups_count.GetHeight(), thread_groups_count.GetDepth() };
    const MTLSize mtl_threads_per_group{ thread_group_size.GetWidth(), thread_group_size.GetHeight(), thread_group_size.GetDepth() };
    [mtl_cmd_encoder dispatchThreadgroups: mtl_thread_groups
                    threadsPerThreadgroup: mtl_threads_per_group];
}

} // namespace Methane::Graphics::Metal
