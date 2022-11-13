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

FILE: Methane/Graphics/Base/CommandKit.h
Methane command kit implementation.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/ICommandKit.h>

#include <map>

namespace Methane::Graphics::Base
{

class CommandKit final
    : public Rhi::ICommandKit
    , public Object
{
public:
    CommandKit(const Rhi::IContext& context, Rhi::CommandListType cmd_list_type);
    explicit CommandKit(Rhi::ICommandQueue& cmd_queue);

    // IObject overrides
    bool SetName(const std::string& name) override;

    // ICommandKit interface
    [[nodiscard]] const Rhi::IContext&  GetContext() const noexcept override  { return m_context; }
    [[nodiscard]] Rhi::CommandListType  GetListType() const noexcept override { return m_cmd_list_type; }
    [[nodiscard]] Rhi::ICommandQueue&   GetQueue() const override;
    [[nodiscard]] bool                  HasList(Rhi::CommandListId cmd_list_id) const noexcept override;
    [[nodiscard]] bool                  HasListWithState(Rhi::CommandListState cmd_list_state, Rhi::CommandListId cmd_list_id) const noexcept override;
    [[nodiscard]] Rhi::ICommandList&    GetList(Rhi::CommandListId cmd_list_id) const override;
    [[nodiscard]] Rhi::ICommandList&    GetListForEncoding(Rhi::CommandListId cmd_list_id, std::string_view debug_group_name) const override;
    [[nodiscard]] Rhi::ICommandListSet& GetListSet(const std::vector<Rhi::CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const override;
    [[nodiscard]] Rhi::IFence&          GetFence(Rhi::CommandListId fence_id) const override;

private:
    using CommandListIndex = uint32_t;
    using CommandListSetId = std::pair<Opt<Data::Index>, uint32_t>;
    using CommandListIndexById = std::map<Rhi::CommandListId, uint32_t>;
    using CommandListSetById = std::map<CommandListSetId, Ptr<Rhi::ICommandListSet>>;

    CommandListIndex GetCommandListIndexById(Rhi::CommandListId cmd_list_id) const noexcept;
    CommandListSetId GetCommandListSetId(const std::vector<Rhi::CommandListId>& cmd_list_ids, Opt<Data::Index> frame_index_opt) const;

    const Rhi::IContext&            m_context;
    Rhi::CommandListType            m_cmd_list_type;
    mutable Ptr<Rhi::ICommandQueue> m_cmd_queue_ptr;
    mutable Ptrs<Rhi::ICommandList> m_cmd_list_ptrs;
    mutable CommandListIndexById    m_cmd_list_index_by_id;
    mutable CommandListSetById      m_cmd_list_set_by_id;
    mutable Ptrs<Rhi::IFence>       m_fence_ptrs;
};

} // namespace Methane::Graphics::Base
