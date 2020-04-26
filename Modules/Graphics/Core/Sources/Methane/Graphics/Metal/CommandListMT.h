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

FILE: Methane/Graphics/DirectX12/CommandListMT.h
Metal command lists sequence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandListBase.h>

namespace Methane::Graphics
{

namespace CommandListMT
{

class DebugGroupMT : public CommandListBase::DebugGroupBase
{
public:
    DebugGroupMT(std::string name);

    const NSString* GetNSName() const noexcept { return m_ns_name; }

private:
    const NSString* m_ns_name;
};

} // namespace CommandListMT

class CommandListsMT final : public CommandListsBase
{
public:
    CommandListsMT(Refs<CommandList> command_list_refs);

private:
};

} // namespace Methane::Graphics
