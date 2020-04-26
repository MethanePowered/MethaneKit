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

FILE: Methane/Graphics/DirectX12/CommandListVK.cpp
Vulkan command lists sequence implementation

******************************************************************************/

#include "CommandListVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<CommandLists> CommandLists::Create(Refs<CommandList> command_list_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListsVK>(std::move(command_list_refs));
}

CommandListsVK::CommandListsVK(Refs<CommandList> command_list_refs)
    : CommandListsBase(std::move(command_list_refs))
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
