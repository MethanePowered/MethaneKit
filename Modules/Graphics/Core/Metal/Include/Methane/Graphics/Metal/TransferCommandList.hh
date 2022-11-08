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

FILE: Methane/Graphics/Metal/TransferCommandList.hh
Metal implementation of the transfer command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"

#include <Methane/Graphics/ITransferCommandList.h>
#include <Methane/Graphics/Base/CommandList.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

class CommandQueue;

class TransferCommandList final
    : public CommandList<id<MTLBlitCommandEncoder>, Base::CommandList>
    , public ITransferCommandList
{
public:
    TransferCommandList(Base::CommandQueue& command_queue);

    // ICommandList interface
    void Reset(ICommandListDebugGroup* p_debug_group = nullptr) override;
};

} // namespace Methane::Graphics::Metal
