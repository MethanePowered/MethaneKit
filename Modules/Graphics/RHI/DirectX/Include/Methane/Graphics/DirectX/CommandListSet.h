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

FILE: Methane/Graphics/DirectX/CommandList.h
DirectX 12 command list accessor interface for template class CommandList<CommandListBaseT>

******************************************************************************/

#pragma once

#include "Fence.h"

#include <Methane/Graphics/Base/CommandListSet.h>

#include <wrl.h>
#include <directx/d3d12.h>
#include <span>
#include <vector>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class CommandQueue;

class CommandListSet final
    : public Base::CommandListSet
{
public:
    explicit CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // Base::CommandListSet interface
    void Execute(const Rhi::ICommandList::CompletedCallback& completed_callback) override;
    void WaitUntilCompleted(uint32_t timeout_ms) override;

    using NativeCommandListSpan = std::span<const ID3D12CommandList* const>;
    NativeCommandListSpan GetNativeCommandLists() const noexcept { return m_native_command_lists; }

    CommandQueue&       GetDirectCommandQueue() noexcept;
    const CommandQueue& GetDirectCommandQueue() const noexcept;

private:
    using NativeCommandLists = std::vector<ID3D12CommandList*>;

    NativeCommandLists m_native_command_lists;
    Fence              m_execution_completed_fence;
};

} // namespace Methane::Graphics::DirectX
