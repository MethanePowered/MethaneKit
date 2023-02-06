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

FILE: Methane/Graphics/DirectX/CommandQueue.h
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandQueueTracking.h>

#pragma warning(push)
#pragma warning(disable: 4189)
#include <tracy/TracyD3D12.hpp>
#pragma warning(pop)

#include <wrl.h>
#include <directx/d3d12.h>

namespace Methane::Graphics::DirectX
{

namespace wrl = Microsoft::WRL;

struct IContext;
class CommandListSet;

class CommandQueue final // NOSONAR - destructor is needed
    : public Base::CommandQueueTracking
{
public:
    CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type);
    ~CommandQueue() override;

    // ICommandQueue interface
    [[nodiscard]] Ptr<Rhi::IFence>                     CreateFence() override;
    [[nodiscard]] Ptr<Rhi::ITransferCommandList>       CreateTransferCommandList() override;
    [[nodiscard]] Ptr<Rhi::IRenderCommandList>         CreateRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::IParallelRenderCommandList> CreateParallelRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::ITimestampQueryPool>        CreateTimestampQueryPool(uint32_t max_timestamps_per_frame) override;
    uint32_t GetFamilyIndex() const noexcept override { return 0U; }

    // IObject interface
    bool SetName(std::string_view name) override;

#if defined(METHANE_GPU_INSTRUMENTATION_ENABLED) && METHANE_GPU_INSTRUMENTATION_ENABLED == 2
    // Base::CommandQueueTracking override
    void CompleteExecution(const Opt<Data::Index>& frame_index = { }) override;
#endif

    const IContext&      GetDirectContext() const noexcept { return m_dx_context; }
    ID3D12CommandQueue&  GetNativeCommandQueue();
    const TracyD3D12Ctx& GetTracyD3D12Ctx() const noexcept { return m_tracy_context; }

private:
    const IContext&                 m_dx_context;
    wrl::ComPtr<ID3D12CommandQueue> m_cp_command_queue;
    TracyD3D12Ctx                   m_tracy_context;
};

} // namespace Methane::Graphics::DirectX
