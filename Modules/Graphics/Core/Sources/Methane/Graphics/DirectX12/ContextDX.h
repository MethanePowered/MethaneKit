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

FILE: Methane/Graphics/DirectX12/ContextDX.cpp
DirectX 12 implementation of the context interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ContextBase.h>

#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

namespace Methane::Graphics
{

namespace wrl = Microsoft::WRL;

class CommandQueueDX;
class DeviceBase;
class DeviceDX;

class ContextDX : public virtual ContextBase
{
public:
    ContextDX(DeviceBase& device, Type type);
    ~ContextDX() override;

    // Context interface
    void WaitForGpu(WaitFor wait_for) override;

    // ContextBase interface
    void Initialize(DeviceBase& device, bool deferred_heap_allocation) override;
    void Release() override;
    void OnCommandQueueCompleted(CommandQueue& cmd_list, uint32_t frame_index) override;

    // Object interface
    void SetName(const std::string& name) override;

    const DeviceDX& GetDeviceDX() const;
    CommandQueueDX& GetUploadCommandQueueDX();

protected:
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

    Ptr<FenceDX> m_sp_upload_fence;
};

} // namespace Methane::Graphics
