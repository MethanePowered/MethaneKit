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

FILE: Methane/Graphics/CommandQueue.h
Methane command queue interface: queues are used to execute command lists.

******************************************************************************/

#pragma once

#include "Object.h"
#include "CommandList.h"
#include "Context.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct CommandQueue : virtual Object
{
    // Create CommandQueue instance
    static Ptr<CommandQueue> Create(Context& context);

    // CommandQueue interface
    virtual void Execute(const Refs<CommandList>& command_lists) = 0;
};

} // namespace Methane::Graphics
