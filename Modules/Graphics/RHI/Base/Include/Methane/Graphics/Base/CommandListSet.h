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

FILE: Methane/Graphics/Base/CommandListSet.h
Base implementation of the command list set interface.

******************************************************************************/

#pragma once

#include "CommandList.h"

#include <Methane/Graphics/RHI/ICommandListSet.h>
#include <Methane/Data/Receiver.hpp>
#include <Methane/Instrumentation.h>

#include <mutex>
#include <atomic>

namespace Methane::Graphics::Base
{

class CommandListSet
    : public Rhi::ICommandListSet
    , protected Data::Receiver<Rhi::IObjectCallback>
    , public std::enable_shared_from_this<CommandListSet>
{
public:
    explicit CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // ICommandListSet overrides
    Data::Size                     GetCount() const noexcept final       { return static_cast<Data::Size>(m_refs.size()); }
    const Refs<Rhi::ICommandList>& GetRefs() const noexcept final        { return m_refs; }
    Rhi::ICommandList&             operator[](Data::Index index) const final;
    const Opt<Data::Index>&        GetFrameIndex() const noexcept final  { return m_frame_index_opt; }
    Ptr<ICommandListSet>           GetPtr() final                        { return shared_from_this(); }

    // CommandListSet interface
    virtual void Execute(const Rhi::ICommandList::CompletedCallback& completed_callback);
    virtual void WaitUntilCompleted(uint32_t timeout_ms = 0U) = 0;

    bool IsExecuting() const noexcept { return m_is_executing; }
    void Complete() const;

    [[nodiscard]] Ptr<CommandListSet>      GetBasePtr()                 { return shared_from_this(); }
    [[nodiscard]] const Refs<CommandList>& GetBaseRefs() const noexcept { return m_base_refs; }
    [[nodiscard]] const CommandList&       GetBaseCommandList(Data::Index index) const;
    [[nodiscard]] CommandQueue&            GetBaseCommandQueue()        { return m_base_refs.back().get().GetBaseCommandQueue(); }
    [[nodiscard]] const CommandQueue&      GetBaseCommandQueue() const  { return m_base_refs.back().get().GetBaseCommandQueue(); }
    [[nodiscard]] const std::string&       GetCombinedName();

protected:
    // IObjectCallback interface
    void OnObjectNameChanged(Rhi::IObject&, const std::string&) override;

private:
    Refs<Rhi::ICommandList> m_refs;
    Refs<CommandList>       m_base_refs;
    Ptrs<CommandList>       m_base_ptrs;
    Opt<Data::Index>        m_frame_index_opt;
    std::string             m_combined_name;

    mutable TracyLockable(std::mutex, m_command_lists_mutex);
    mutable std::atomic<bool> m_is_executing = false;
};

} // namespace Methane::Graphics::Base
