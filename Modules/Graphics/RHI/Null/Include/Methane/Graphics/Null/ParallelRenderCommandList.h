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
    void SetBeginningResourceBarriers(const Rhi::IResourceBarriers&) override { /* Intentionally unimplemented */ }
    void SetEndingResourceBarriers(const Rhi::IResourceBarriers&) override    { /* Intentionally unimplemented */ }

private:
    // ParallelRenderCommandListBase interface
    [[nodiscard]] Ptr<Rhi::IRenderCommandList> CreateCommandList(bool is_beginning_list) override;
};

} // namespace Methane::Graphics::Null
