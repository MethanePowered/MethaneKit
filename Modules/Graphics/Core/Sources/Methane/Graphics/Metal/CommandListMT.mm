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

FILE: Methane/Graphics/DirectX12/CommandListMT.mm
Metal command lists sequence implementation

******************************************************************************/

#include "CommandListMT.h"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<CommandList::DebugGroup> CommandList::DebugGroup::Create(std::string name)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListMT::DebugGroupMT>(std::move(name));
}

CommandListMT::DebugGroupMT::DebugGroupMT(std::string name)
    : CommandListBase::DebugGroupBase(std::move(name))
    , m_ns_name(MacOS::ConvertToNsType<std::string, NSString*>(name))
{
    META_FUNCTION_TASK();
}

Ptr<CommandLists> CommandLists::Create(Refs<CommandList> command_list_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListsMT>(std::move(command_list_refs));
}

CommandListsMT::CommandListsMT(Refs<CommandList> command_list_refs)
    : CommandListsBase(std::move(command_list_refs))
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
