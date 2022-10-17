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

FILE: Methane/Graphics/CommandKitBase.h
Methane command kit implementation.

******************************************************************************/

#pragma once

#include "ObjectBase.h"

#include <Methane/Graphics/CommandKit.h>

#include <map>

namespace Methane::Graphics
{

class CommandKitBase final
    : public CommandKit
    , public ObjectBase
{
public:
    CommandKitBase(const Context& context, CommandList::Type cmd_list_type);
    explicit CommandKitBase(CommandQueue& cmd_queue);

    // IObject overrides
    bool SetName(const std::string& name) override;

    // CommandKit interface
    [[nodiscard]] const Context&    GetContext() const noexcept override  { return m_context; }
    [[nodiscard]] CommandList::Type GetListType() const noexcept override { return m_cmd_list_type; }
    [[nodiscard]] CommandQueue&     GetQueue() const override;
    [[nodiscard]] bool              HasList(CommandListId cmd_list_id) const noexcept override;
    [[nodiscard]] bool              HasListWithState(CommandList::State cmd_list_state, CommandListId cmd_list_id) const noexcept override;
    [[nodiscard]] CommandList&      GetList(CommandListId cmd_list_id) const override;
    [[nodiscard]] CommandList&      GetListForEncoding(CommandListId cmd_list_id, std::string_view debug_group_name) const override;
    [[nodiscard]] CommandListSet&   GetListSet(const std::vector<CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const override;
    [[nodiscard]] IFence&           GetFence(CommandListId fence_id) const override;

private:
    using CommandListIndex = uint32_t;
    using CommandListSetId = std::pair<Opt<Data::Index>, uint32_t>;
    using CommandListIndexById = std::map<CommandListId, uint32_t>;
    using CommandListSetById = std::map<CommandListSetId, Ptr<CommandListSet>>;

    CommandListIndex GetCommandListIndexById(CommandListId cmd_list_id) const noexcept;
    CommandListSetId GetCommandListSetId(const std::vector<CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const;

    const Context&               m_context;
    CommandList::Type            m_cmd_list_type;
    mutable Ptr<CommandQueue>    m_cmd_queue_ptr;
    mutable Ptrs<CommandList>    m_cmd_list_ptrs;
    mutable CommandListIndexById m_cmd_list_index_by_id;
    mutable CommandListSetById   m_cmd_list_set_by_id;
    mutable Ptrs<IFence>         m_fence_ptrs;
};

} // namespace Methane::Graphics
