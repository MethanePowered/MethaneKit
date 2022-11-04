/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/FenceMT.hh
Metal fence implementation.

******************************************************************************/

#pragma once

#include "../../../../Base/Include/Methane/Graphics/FenceBase.h"
#include "../../../../../../Common/Instrumentation/Include/Methane/Instrumentation.h"

#include "../../../../../../../../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.0.sdk/usr/include/c++/v1/mutex"
#include "../../../../../../../../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.0.sdk/usr/include/c++/v1/condition_variable"

#import "../../../../../../../../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.0.sdk/System/Library/Frameworks/Metal.framework/Headers/Metal.h"

namespace Methane::Graphics
{

class CommandQueueMT;

class FenceMT final : public FenceBase
{
public:
    explicit FenceMT(CommandQueueBase& command_queue);

    // IFence overrides
    void Signal() override;
    void WaitOnCpu() override;
    void WaitOnGpu(ICommandQueue& wait_on_command_queue) override;

    // IObject override
    bool SetName(const std::string& name) override;

private:
    CommandQueueMT& GetCommandQueueMT();
    
    static const dispatch_queue_t& GetDispatchQueue();

    id<MTLSharedEvent>          m_mtl_event;
    MTLSharedEventListener*     m_mtl_event_listener;
    TracyLockable(std::mutex,   m_wait_mutex)
    std::condition_variable_any m_wait_condition_var;
    bool                        m_is_signalled = false;
};

} // namespace Methane::Graphics
