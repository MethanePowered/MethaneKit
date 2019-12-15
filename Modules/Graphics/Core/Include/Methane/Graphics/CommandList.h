/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

#include "Types.h"
#include "Object.h"
#include "Resource.h"
#include "Program.h"

#include <memory>
#include <string>
#include <vector>

namespace Methane::Graphics
{

struct CommandQueue;

struct CommandList : virtual Object
{
    using Ptr  = std::shared_ptr<CommandList>;
    using Ref  = std::reference_wrapper<CommandList>;
    using Refs = std::vector<Ref>;

    enum class Type : uint32_t
    {
        RenderCommandList = 0u,
        ParallelRenderCommandList,
    };

    // CommandList interface
    virtual Type GetType() const = 0;
    virtual void PushDebugGroup(const std::string& name) = 0;
    virtual void PopDebugGroup() = 0;
    virtual void SetResourceBindings(Program::ResourceBindings& resource_bindings) = 0;
    virtual void Commit(bool present_drawable) = 0;
    virtual CommandQueue& GetCommandQueue() = 0;

    virtual ~CommandList() = default;
};

} // namespace Methane::Graphics
