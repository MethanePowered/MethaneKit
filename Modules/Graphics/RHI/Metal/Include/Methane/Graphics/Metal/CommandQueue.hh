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
    uint32_t GetFamilyIndex() const noexcept override { return 0U; }

    // IObject interface
    bool SetName(const std::string& name) override;
    
    const IContext& GetMetalContext() const noexcept;
    const RenderContext& GetMetalRenderContext() const;
    
    const id<MTLCommandQueue>&  GetNativeCommandQueue() const { return m_mtl_command_queue; }

private:
    void Reset();
    
    id<MTLCommandQueue> m_mtl_command_queue = nil;
};

} // namespace Methane::Graphics::Metal
