/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/CommandKit.h
Methane command kit interface: provides unified toolkit for commands execution
and synchronization within a stored command queue.

******************************************************************************/

#pragma once

#include "Object.h"
#include "CommandList.h"

#include <limits>
#include <vector>
#include <string_view>

namespace Methane::Graphics
{

struct Context;
struct CommandQueue;
struct Fence;

struct CommandKit : virtual Object // NOSONAR
{
    using CommandListId = uint32_t;

    enum class CommandListPurpose : CommandListId
    {
        Default        = 0U,
        PreUploadSync  = std::numeric_limits<CommandListId>::max() - 2,
        PostUploadSync
    };

    // Create CommandKit instance
    [[nodiscard]] static Ptr<CommandKit> Create(const Context& context, CommandList::Type command_lists_type);
    [[nodiscard]] static Ptr<CommandKit> Create(CommandQueue& cmd_queue);

    // CommandKit interface
    [[nodiscard]] virtual const Context&    GetContext() const noexcept = 0;
    [[nodiscard]] virtual CommandQueue&     GetQueue() const = 0;
    [[nodiscard]] virtual CommandList::Type GetListType() const noexcept = 0;
    [[nodiscard]] virtual bool              HasList(CommandListId cmd_list_id = 0U) const noexcept = 0;
    [[nodiscard]] virtual bool              HasListWithState(CommandList::State cmd_list_state, CommandListId cmd_list_id = 0U) const noexcept = 0;
    [[nodiscard]] virtual CommandList&      GetList(CommandListId cmd_list_id = 0U) const = 0;
    [[nodiscard]] virtual CommandList&      GetListForEncoding(CommandListId cmd_list_id = 0U, std::string_view debug_group_name = {}) const = 0;
    [[nodiscard]] virtual CommandListSet&   GetListSet(const std::vector<CommandListId>& cmd_list_ids = { 0U }, Opt<Data::Index> frame_index_opt = {}) const = 0;
    [[nodiscard]] virtual Fence&            GetFence(CommandListId fence_id = 0U) const = 0;
};

} // namespace Methane::Graphics
