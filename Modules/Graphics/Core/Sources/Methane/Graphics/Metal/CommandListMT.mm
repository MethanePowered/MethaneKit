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

FILE: Methane/Graphics/DirectX12/CommandListMT.mm
Metal command lists sequence implementation

******************************************************************************/

#include "CommandListMT.hh"

#include <Methane/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

namespace Methane::Graphics
{

Ptr<CommandList::DebugGroup> CommandList::DebugGroup::Create(const std::string& name)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListDebugGroupMT>(name);
}

CommandListDebugGroupMT::CommandListDebugGroupMT(const std::string& name)
    : CommandListBase::DebugGroupBase(std::move(name))
    , m_ns_name(MacOS::ConvertToNsType<std::string, NSString*>(ObjectBase::GetName()))
{
    META_FUNCTION_TASK();
}

Ptr<CommandListSet> CommandListSet::Create(const Refs<CommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListSetMT>(command_list_refs, frame_index_opt);
}

CommandListSetMT::CommandListSetMT(const Refs<CommandList>& command_list_refs, Opt<Data::Index> frame_index_opt)
    : CommandListSetBase(command_list_refs, frame_index_opt)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
