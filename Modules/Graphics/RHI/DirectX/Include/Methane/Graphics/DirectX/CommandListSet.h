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

#include <Methane/Graphics/Base/CommandList.h>

#include <wrl.h>
#include <directx/d3d12.h>

#include <mutex>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class CommandQueue;

class CommandListDebugGroup final
    : public Base::CommandListDebugGroup
{
public:
    explicit CommandListDebugGroup(const std::string& name);

    const std::wstring& GetWideName() const noexcept { return m_wide_name; }

private:
    const std::wstring m_wide_name;
};

struct ICommandListDx
{
    using DebugGroup = CommandListDebugGroup;

    virtual CommandQueue&               GetDirectCommandQueue() = 0;
    virtual ID3D12GraphicsCommandList&  GetNativeCommandList() const = 0;
    virtual ID3D12GraphicsCommandList4* GetNativeCommandList4() const = 0;
    virtual void SetResourceBarriers(const IResourceBarriers& resource_barriers) = 0;

    virtual ~ICommandListDx() = default;
};

class CommandListSet final
    : public Base::CommandListSet
{
public:
public:
    explicit CommandListSet(const Refs<ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    // Base::CommandListSet interface
    void Execute(const ICommandList::CompletedCallback& completed_callback) override;
    void WaitUntilCompleted() override;

    using NativeCommandLists = std::vector<ID3D12CommandList*>;
    const NativeCommandLists& GetNativeCommandLists() const noexcept { return m_native_command_lists; }

    CommandQueue&       GetDirectCommandQueue() noexcept;
    const CommandQueue& GetDirectCommandQueue() const noexcept;

private:
    NativeCommandLists m_native_command_lists;
    Fence              m_execution_completed_fence;
};

} // namespace Methane::Graphics::DirectX
