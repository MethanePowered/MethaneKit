/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/CommandQueue.h
Null implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "QueryPool.h"

#include <Methane/Graphics/Base/CommandQueue.h>

namespace Methane::Graphics::Null
{

struct IFence;

class CommandQueue final
    : public Base::CommandQueue
{
public:
    using Base::CommandQueue::CommandQueue;

    // ICommandQueue interface
    [[nodiscard]] Ptr<Rhi::IFence>                     CreateFence() override;
    [[nodiscard]] Ptr<Rhi::ITransferCommandList>       CreateTransferCommandList() override;
    [[nodiscard]] Ptr<Rhi::IComputeCommandList>        CreateComputeCommandList() override;
    [[nodiscard]] Ptr<Rhi::IRenderCommandList>         CreateRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::IParallelRenderCommandList> CreateParallelRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::ITimestampQueryPool>        CreateTimestampQueryPool(uint32_t max_timestamps_per_frame) override;
    uint32_t                                           GetFamilyIndex() const noexcept override { return 0U; }
    const Ptr<Rhi::ITimestampQueryPool>&               GetTimestampQueryPoolPtr() override      { return m_timestamp_query_pool_ptr; }

private:
    const Ptr<Rhi::ITimestampQueryPool> m_timestamp_query_pool_ptr = std::make_shared<TimestampQueryPool>(*this, 1000U);
};

} // namespace Methane::Graphics::Null
