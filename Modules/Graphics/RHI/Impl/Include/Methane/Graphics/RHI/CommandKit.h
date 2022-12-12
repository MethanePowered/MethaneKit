/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/CommandKit.h
Methane CommandKit PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/ICommandKit.h>

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderContext;
class RenderCommandList;
class CommandListSet;

class CommandKit
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandKit);

    CommandKit(const Ptr<ICommandKit>& interface_ptr);
    CommandKit(ICommandKit& interface_ref);
    CommandKit(const CommandQueue& command_queue);
    CommandKit(const RenderContext& context, CommandListType command_lists_type);

    void Init(const CommandQueue& command_queue);
    void Init(const RenderContext& context, CommandListType command_lists_type);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ICommandKit& GetInterface() const META_PIMPL_NOEXCEPT;

    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    [[nodiscard]] const IContext&   GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] CommandQueue      GetQueue() const;
    [[nodiscard]] CommandListType   GetListType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] bool              HasList(CommandListId cmd_list_id = 0U) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] bool              HasListWithState(CommandListState cmd_list_state, CommandListId cmd_list_id = 0U) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] RenderCommandList GetRenderList(CommandListId cmd_list_id = 0U) const;
    [[nodiscard]] RenderCommandList GetRenderListForEncoding(CommandListId cmd_list_id = 0U, std::string_view debug_group_name = {}) const;
    [[nodiscard]] CommandListSet    GetListSet(const std::vector<CommandListId>& cmd_list_ids = { 0U }, Opt<Data::Index> frame_index_opt = {}) const;
    [[nodiscard]] IFence&           GetFence(CommandListId fence_id = 0U) const;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
