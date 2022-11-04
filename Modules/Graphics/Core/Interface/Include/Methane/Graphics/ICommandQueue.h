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

FILE: Methane/Graphics/ICommandQueue.h
Methane command queue interface: queues are used to execute command lists.

******************************************************************************/

#pragma once

#include "IObject.h"
#include "ICommandList.h"
#include "IContext.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct ICommandQueue
    : virtual IObject // NOSONAR
{
    // Create ICommandQueue instance
    [[nodiscard]] static Ptr<ICommandQueue> Create(const IContext& context, CommandListType command_lists_type);

    // ICommandQueue interface
    [[nodiscard]] virtual const IContext&   GetContext() const noexcept = 0;
    [[nodiscard]] virtual CommandListType   GetCommandListType() const noexcept = 0;
    [[nodiscard]] virtual uint32_t          GetFamilyIndex() const noexcept = 0;
    virtual void Execute(ICommandListSet& command_lists, const ICommandList::CompletedCallback& completed_callback = {}) = 0;
};

} // namespace Methane::Graphics
