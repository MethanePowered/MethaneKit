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

FILE: Methane/Graphics/Null/ParallelRenderCommandList.h
Null implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include "RenderPass.h"

#include <Methane/Graphics/Base/ParallelRenderCommandList.h>

namespace Methane::Graphics::Null
{

class CommandQueue;

class ParallelRenderCommandList final
    : public Base::ParallelRenderCommandList
{
public:
    using Base::ParallelRenderCommandList::ParallelRenderCommandList;

    // IParallelRenderCommandList interface
    void SetBeginningResourceBarriers(const Rhi::IResourceBarriers& barriers) override;
    void SetEndingResourceBarriers(const Rhi::IResourceBarriers& barriers) override;

    const Rhi::IResourceBarriers* GetBeginningResourceBarriers() const noexcept { return m_beginning_barriers_ptr; }
    const Rhi::IResourceBarriers* GetEndingResourceBarriers() const noexcept    { return m_ending_barriers_ptr; }

private:
    // ParallelRenderCommandListBase interface
    [[nodiscard]] Ptr<Rhi::IRenderCommandList> CreateCommandList(bool is_beginning_list) override;

    const Rhi::IResourceBarriers* m_beginning_barriers_ptr = nullptr;
    const Rhi::IResourceBarriers* m_ending_barriers_ptr = nullptr;
};

} // namespace Methane::Graphics::Null
