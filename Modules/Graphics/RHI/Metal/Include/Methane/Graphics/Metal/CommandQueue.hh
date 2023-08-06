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

FILE: Methane/Graphics/Metal/CommandQueue.hh
Metal implementation of the command queue interface.

******************************************************************************/

#pragma once

#include "QueryPool.hh"

#include <Methane/Graphics/Base/CommandQueue.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

struct IContext;
class RenderContext;

class CommandQueue final : public Base::CommandQueue
{
public:
    CommandQueue(const Base::Context& context, Rhi::CommandListType command_lists_type);

    // ICommandQueue interface
    [[nodiscard]] Ptr<Rhi::IFence>                     CreateFence() override;
    [[nodiscard]] Ptr<Rhi::ITransferCommandList>       CreateTransferCommandList() override;
    [[nodiscard]] Ptr<Rhi::IComputeCommandList>        CreateComputeCommandList() override;
    [[nodiscard]] Ptr<Rhi::IRenderCommandList>         CreateRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::IParallelRenderCommandList> CreateParallelRenderCommandList(Rhi::IRenderPass& render_pass) override;
    [[nodiscard]] Ptr<Rhi::ITimestampQueryPool>        CreateTimestampQueryPool(uint32_t max_timestamps_per_frame) override;
    uint32_t                                           GetFamilyIndex() const noexcept override { return 0U; }
    const Ptr<Rhi::ITimestampQueryPool>&               GetTimestampQueryPoolPtr() override      { return m_timestamp_query_pool_ptr; }

    // IObject interface
    bool SetName(std::string_view name) override;
    
    const IContext& GetMetalContext() const noexcept;
    const RenderContext& GetMetalRenderContext() const;
    
    const id<MTLCommandQueue>&  GetNativeCommandQueue() const { return m_mtl_command_queue; }

private:
    void Reset();
    
    id<MTLCommandQueue> m_mtl_command_queue = nil;
    Ptr<Rhi::ITimestampQueryPool> m_timestamp_query_pool_ptr = std::make_shared<TimestampQueryPool>(*this, 1000U);
};

} // namespace Methane::Graphics::Metal
