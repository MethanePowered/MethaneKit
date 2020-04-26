/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
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
#include "BlitCommandListDX.h"
#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<CommandLists> CommandLists::Create(Refs<CommandList> command_list_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListsDX>(std::move(command_list_refs));
}

CommandListsDX::CommandListsDX(Refs<CommandList> command_list_refs)
    : CommandListsBase(std::move(command_list_refs))
{
    META_FUNCTION_TASK();

    const Refs<CommandListBase>& base_command_list_refs = GetBaseRefs();

    m_native_command_lists.reserve(base_command_list_refs.size());
    for(const Ref<CommandListBase>& command_list_ref : base_command_list_refs)
    {
        CommandListBase& command_list = command_list_ref.get();
        switch (command_list.GetType())
        {
        case CommandList::Type::Blit:
        {
            m_native_command_lists.emplace_back(&static_cast<BlitCommandListDX&>(command_list).GetNativeCommandList());
        } break;

        case CommandList::Type::Render:
        {
            m_native_command_lists.emplace_back(&static_cast<RenderCommandListDX&>(command_list).GetNativeCommandList());
        } break;

        case CommandList::Type::ParallelRender:
        {
            const NativeCommandLists native_command_lists = static_cast<ParallelRenderCommandListDX&>(command_list).GetNativeCommandLists();
            m_native_command_lists.insert(m_native_command_lists.end(), native_command_lists.begin(), native_command_lists.end());
        } break;
        }
    }
}

} // namespace Methane::Graphics
