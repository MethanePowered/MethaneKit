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

FILE: Methane/Graphics/DirectX/CommandList.cpp
DirectX 12 command lists collection implementation

******************************************************************************/

#include <Methane/Graphics/DirectX/CommandListSet.h>
#include <Methane/Graphics/DirectX/ParallelRenderCommandList.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<ICommandListSet> ICommandListSet::Create(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::CommandListSet>(command_list_refs, frame_index_opt);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

CommandListSet::CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : Base::CommandListSet(command_list_refs, frame_index_opt)
    , m_execution_completed_fence(GetBaseCommandQueue())
{
    META_FUNCTION_TASK();

    const Refs<Base::CommandList>& base_command_list_refs = GetBaseRefs();

    std::stringstream fence_name_ss;
    fence_name_ss << "Execution completed for command list set:";

    m_native_command_lists.reserve(base_command_list_refs.size());
    for(const Ref<Base::CommandList>& command_list_ref : base_command_list_refs)
    {
        const Base::CommandList& command_list = command_list_ref.get();
        if (command_list.GetType() == Rhi::CommandListType::ParallelRender)
        {
            const CommandListSet::NativeCommandLists parallel_native_cmd_lists = static_cast<const ParallelRenderCommandList&>(command_list).GetNativeCommandLists();
            m_native_command_lists.insert(m_native_command_lists.end(), parallel_native_cmd_lists.begin(), parallel_native_cmd_lists.end());
        }
        else
        {
            m_native_command_lists.emplace_back(&dynamic_cast<const ICommandList&>(command_list).GetNativeCommandList());
        }
        fence_name_ss << " '" << command_list.GetName() << "'";
    }

    m_execution_completed_fence.SetName(fence_name_ss.str());
}

void CommandListSet::Execute(const Rhi::ICommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    Base::CommandListSet::Execute(completed_callback);
    GetDirectCommandQueue().GetNativeCommandQueue().ExecuteCommandLists(static_cast<UINT>(m_native_command_lists.size()), m_native_command_lists.data());
    m_execution_completed_fence.Signal();
}

void CommandListSet::WaitUntilCompleted(uint32_t /*timeout_ms*/)
{
    META_FUNCTION_TASK();
    m_execution_completed_fence.WaitOnCpu();
    Complete();
}

CommandQueue& CommandListSet::GetDirectCommandQueue() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueue&>(GetBaseCommandQueue());
}

const CommandQueue& CommandListSet::GetDirectCommandQueue() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const CommandQueue&>(GetBaseCommandQueue());
}

} // namespace Methane::Graphics::DirectX
