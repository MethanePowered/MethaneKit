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
    CommandKitBase(Context& context, CommandList::Type cmd_list_type);
    explicit CommandKitBase(CommandQueue& cmd_queue);

    // Object overrides
    void SetName(const std::string& name) override;

    // CommandKit interface
    [[nodiscard]] Context&          GetContext() noexcept override        { return m_context; }
    [[nodiscard]] CommandList::Type GetListType() const noexcept override { return m_cmd_list_type; }
    [[nodiscard]] CommandQueue&     GetQueue() const override;
    [[nodiscard]] bool              HasList(uint32_t cmd_list_id) const noexcept override;
    [[nodiscard]] bool              HasListWithState(CommandList::State cmd_list_state, uint32_t cmd_list_id) const noexcept override;
    [[nodiscard]] CommandList&      GetList(uint32_t cmd_list_id) const override;
    [[nodiscard]] CommandList&      GetListForEncoding(uint32_t cmd_list_id, std::string_view debug_group_name) const override;
    [[nodiscard]] CommandListSet&   GetListSet(const std::vector<uint32_t>& cmd_list_ids) const override;
    [[nodiscard]] Fence&            GetFence(uint32_t fence_id) const override;

private:
    using CommandListSetById = std::map<uint32_t, Ptr<CommandListSet>>;

    Context&                   m_context;
    CommandList::Type          m_cmd_list_type;
    mutable Ptr<CommandQueue>  m_cmd_queue_ptr;
    mutable Ptrs<CommandList>  m_cmd_list_ptrs;
    mutable CommandListSetById m_cmd_list_set_by_id;
    mutable Ptrs<Fence>        m_fence_ptrs;
};

} // namespace Methane::Graphics
