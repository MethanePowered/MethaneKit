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

FILE: Methane/Graphics/CommandList.h
Methane command list interface: this is uncreatable common command list interface,
to create instance refer to RenderCommandList, etc. for specific derived interface.

******************************************************************************/

#pragma once

#include "Object.h"
#include "ProgramBindings.h"

#include <string>

namespace Methane::Graphics
{

struct CommandQueue;

struct CommandList : virtual Object
{
    enum class Type : uint32_t
    {
        Blit = 0u,
        Render,
        ParallelRender,
    };

    // CommandList interface
    virtual Type GetType() const = 0;
    virtual void PushDebugGroup(const std::string& name) = 0;
    virtual void PopDebugGroup() = 0;
    virtual void Reset(const std::string& debug_group = "") = 0;
    virtual void SetProgramBindings(ProgramBindings& program_bindings,
                                    ProgramBindings::ApplyBehavior::Mask apply_behavior = ProgramBindings::ApplyBehavior::AllIncremental) = 0;
    virtual void Commit() = 0;
    virtual CommandQueue& GetCommandQueue() = 0;

    virtual ~CommandList() = default;
};

} // namespace Methane::Graphics
