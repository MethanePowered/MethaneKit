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

FILE: Methane/Graphics/Metal/CommandQueueMT.hh
Metal implementation of the command queue interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/CommandQueueBase.h>

#import <Metal/Metal.h>

namespace Methane::Graphics
{

struct IContextMT;
class RenderContextMT;

class CommandQueueMT final : public CommandQueueBase
{
public:
    CommandQueueMT(ContextBase& context);
    ~CommandQueueMT() override;

    // Object interface
    void SetName(const std::string& name) override;
    
    IContextMT& GetContextMT() noexcept;
    RenderContextMT& GetRenderContextMT();
    
    id<MTLCommandQueue>&  GetNativeCommandQueue() noexcept { return m_mtl_command_queue; }

private:
    void Reset();
    
    id<MTLCommandQueue>  m_mtl_command_queue = nil;
};

} // namespace Methane::Graphics
