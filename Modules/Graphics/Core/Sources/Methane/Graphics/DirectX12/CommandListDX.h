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

FILE: Methane/Graphics/DirectX12/CommandListDX.h
DirectX 12 command list accessor interface for template class CommandListDX<CommandListBaseT>

******************************************************************************/

#pragma once

#include "FenceDX.h"

#include <Methane/Graphics/CommandListBase.h>

#include <wrl.h>
#include <d3d12.h>

#include <mutex>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;

struct ICommandListDX
{
    class DebugGroupDX : public CommandListBase::DebugGroupBase
    {
    public:
        DebugGroupDX(std::string name);

        const std::wstring& GetWideName() const noexcept { return m_wide_name; }

    private:
        const std::wstring m_wide_name;
    };

    virtual CommandQueueDX&             GetCommandQueueDX() = 0;
    virtual ID3D12GraphicsCommandList&  GetNativeCommandList() const = 0;
    virtual ID3D12GraphicsCommandList4* GetNativeCommandList4() const = 0;
    virtual void SetResourceBarriersDX(const ResourceBase::Barriers& resource_barriers) = 0;

    virtual ~ICommandListDX() = default;
};

class CommandListSetDX final : public CommandListSetBase
{
public:
    CommandListSetDX(Refs<CommandList> command_list_refs);

    // CommandListSetBase interface
    void Execute(uint32_t frame_index, const CommandList::CompletedCallback& completed_callback) final;

    void WaitUntilCompleted() noexcept;

    using NativeCommandLists = std::vector<ID3D12CommandList*>;
    const NativeCommandLists& GetNativeCommandLists() const noexcept { return m_native_command_lists; }

    CommandQueueDX&       GetCommandQueueDX() noexcept;
    const CommandQueueDX& GetCommandQueueDX() const noexcept;

    FenceDX& GetExecutionCompletedFenceDX() noexcept { return m_execution_completed_fence; }

private:
    NativeCommandLists m_native_command_lists;
    FenceDX            m_execution_completed_fence;
};

} // namespace Methane::Graphics
