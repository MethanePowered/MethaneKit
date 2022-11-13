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

FILE: Methane/Graphics/DirectX/Fence.h
DirectX 12 fence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Fence.h>

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

class CommandQueue;

class Fence final // NOSONAR - custom destructor is required
    : public Base::Fence
{
public:
    explicit Fence(Base::CommandQueue& command_queue);
    Fence(const Fence&) = delete;
    Fence(Fence&&) noexcept = default;
    ~Fence() override;

    Fence& operator=(const Fence&) = delete;
    Fence& operator=(Fence&&) noexcept = default;

    // IFence overrides
    void Signal() override;
    void WaitOnCpu() override;
    void WaitOnGpu(Rhi::ICommandQueue& wait_on_command_queue) override;

    // IObject override
    bool SetName(const std::string& name) override;

private:
    CommandQueue& GetDirectCommandQueue();

    wrl::ComPtr<ID3D12Fence> m_cp_fence;
    HANDLE                   m_event = nullptr;
};

} // namespace Methane::Graphics::DirectX
