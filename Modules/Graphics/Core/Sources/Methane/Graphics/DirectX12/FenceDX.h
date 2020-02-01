/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/DirectX12/FenceDX.cpp
DirectX 12 fence wrapper.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ObjectBase.h>

#include <string>
#include <wrl.h>
#include <d3d12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;

class FenceDX : public ObjectBase
{
public:
    FenceDX(CommandQueueDX& command_queue);
    ~FenceDX();

    void Signal();
    void Wait();
    void Flush();

    // Object interface
    void SetName(const std::string& name) noexcept override;

private:
    CommandQueueDX&          m_command_queue;
    uint64_t                 m_value = 0;
    wrl::ComPtr<ID3D12Fence> m_cp_fence;
    HANDLE                   m_event = nullptr;
};

class FrameFenceDX : public FenceDX
{
public:
    FrameFenceDX(CommandQueueDX& command_queue, uint32_t frame);

    uint32_t GetFrame() const noexcept { return m_frame; }

private:
    const uint32_t m_frame = 0;
};

} // namespace Methane::Graphics
