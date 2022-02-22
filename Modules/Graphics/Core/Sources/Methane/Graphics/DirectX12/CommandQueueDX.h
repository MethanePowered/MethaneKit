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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.h
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "QueryBufferDX.h"

#include <Methane/Graphics/CommandQueueTrackingBase.h>

#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

struct IContextDX;
class CommandListSetDX;

class CommandQueueDX final : public CommandQueueTrackingBase
{
public:
    CommandQueueDX(const ContextBase& context, CommandList::Type command_lists_type);

    // CommandQueue interface
    uint32_t GetFamilyIndex() const noexcept override { return 0U; }

    // Object interface
    bool SetName(const std::string& name) override;

    const IContextDX&       GetContextDX() const noexcept;
    ID3D12CommandQueue&     GetNativeCommandQueue();
    TimestampQueryBuffer*   GetTimestampQueryBuffer() noexcept { return m_timestamp_query_buffer_ptr.get(); }

private:
    wrl::ComPtr<ID3D12CommandQueue>   m_cp_command_queue;
    Ptr<TimestampQueryBuffer>         m_timestamp_query_buffer_ptr;
};

} // namespace Methane::Graphics
