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

FILE: Methane/Graphics/DirectX12/CommandListMT.h
Metal command lists sequence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandListBase.h>

#import <Cocoa/Cocoa.h>

namespace Methane::Graphics
{

class CommandListDebugGroupMT final : public CommandListBase::DebugGroupBase
{
public:
    explicit CommandListDebugGroupMT(const std::string& name);

    NSString* _Nonnull GetNSName() const noexcept { return m_ns_name; }

private:
    NSString* _Nonnull m_ns_name;
};

class CommandListSetMT final : public CommandListSetBase
{
public:
    explicit CommandListSetMT(const Refs<CommandList>& command_list_refs);

    virtual void WaitUntilCompleted()
    {
        // Command list execution tracking is not needed in Metal,
        // because native API has command list wait mechanism used directly in CommandListMT::Execute(...)
    }
};

} // namespace Methane::Graphics
