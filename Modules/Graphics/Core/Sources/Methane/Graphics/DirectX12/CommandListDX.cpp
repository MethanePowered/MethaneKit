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

FILE: Methane/Graphics/DirectX12/CommandListDX.cpp
DirectX 12 command lists collection implementation

******************************************************************************/

#include "CommandListDX.h"
#include "ParallelRenderCommandListDX.h"

#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <nowide/convert.hpp>

namespace Methane::Graphics
{

Ptr<CommandList::DebugGroup> CommandList::DebugGroup::Create(const std::string& name)
{
    META_FUNCTION_TASK();
    return std::make_shared<ICommandListDX::DebugGroupDX>(name);
}

ICommandListDX::DebugGroupDX::DebugGroupDX(const std::string& name)
    : CommandListBase::DebugGroupBase(name)
    , m_wide_name(nowide::widen(ObjectBase::GetName()))
{
    META_FUNCTION_TASK();
}

Ptr<CommandListSet> CommandListSet::Create(const Refs<CommandList>& command_list_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListSetDX>(command_list_refs);
}

CommandListSetDX::CommandListSetDX(const Refs<CommandList>& command_list_refs)
    : CommandListSetBase(command_list_refs)
    , m_execution_completed_fence(GetCommandQueueBase())
{
    META_FUNCTION_TASK();

    const Refs<CommandListBase>& base_command_list_refs = GetBaseRefs();

    std::stringstream fence_name_ss;
    fence_name_ss << "Execution completed for command list set: ";

    m_native_command_lists.reserve(base_command_list_refs.size());
    for(const Ref<CommandListBase>& command_list_ref : base_command_list_refs)
    {
        const CommandListBase& command_list = command_list_ref.get();
        if (command_list.GetType() == CommandList::Type::ParallelRender)
        {
            const CommandListSetDX::NativeCommandLists parallel_native_cmd_lists = static_cast<const ParallelRenderCommandListDX&>(command_list).GetNativeCommandLists();
            m_native_command_lists.insert(m_native_command_lists.end(), parallel_native_cmd_lists.begin(), parallel_native_cmd_lists.end());
        }
        else
        {
            m_native_command_lists.emplace_back(&dynamic_cast<const ICommandListDX&>(command_list).GetNativeCommandList());
        }
        fence_name_ss << " '" << command_list.GetName() << "'";
    }

    m_execution_completed_fence.SetName(fence_name_ss.str());
}

void CommandListSetDX::Execute(uint32_t frame_index, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandListSetBase::Execute(frame_index, completed_callback);
    GetCommandQueueDX().GetNativeCommandQueue().ExecuteCommandLists(static_cast<UINT>(m_native_command_lists.size()), m_native_command_lists.data());
    m_execution_completed_fence.Signal();
}

void CommandListSetDX::WaitUntilCompleted() noexcept
{
    META_FUNCTION_TASK();
    m_execution_completed_fence.WaitOnCpu();
    Complete();
}

CommandQueueDX& CommandListSetDX::GetCommandQueueDX() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueDX&>(GetCommandQueueBase());
}

const CommandQueueDX& CommandListSetDX::GetCommandQueueDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const CommandQueueDX&>(GetCommandQueueBase());
}

} // namespace Methane::Graphics
