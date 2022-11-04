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

FILE: Methane/Graphics/Metal/TransferCommandListMT.hh
Metal implementation of the transfer command list interface.

******************************************************************************/

#pragma once

#include "../../../Sources/Methane/Graphics/CommandListMT.hpp"

#include "../../../../Interface/Include/Methane/Graphics/ITransferCommandList.h"
#include "../../../../Base/Include/Methane/Graphics/CommandListBase.h"

#import "../../../../../../../../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.0.sdk/System/Library/Frameworks/Metal.framework/Headers/Metal.h"

namespace Methane::Graphics
{

class CommandQueueMT;

class TransferCommandListMT final
    : public CommandListMT<id<MTLBlitCommandEncoder>, CommandListBase>
    , public ITransferCommandList
{
public:
    TransferCommandListMT(CommandQueueBase& command_queue);

    // ICommandList interface
    void Reset(ICommandListDebugGroup* p_debug_group = nullptr) override;
};

} // namespace Methane::Graphics
