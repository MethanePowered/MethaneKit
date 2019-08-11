/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/ContextMT.h
Metal implementation of the context interface.

******************************************************************************/

#pragma once

#include "../ContextBase.h"

#import <Methane/Platform/MacOS/AppViewMT.h>

#import <Metal/Metal.h>

namespace Methane
{
namespace Graphics
{

struct CommandQueue;
class RenderPassMT;

class ContextMT : public ContextBase
{
public:
    ContextMT(const Platform::AppEnvironment& env, const Data::Provider& data_provider, const Settings& settings);
    ~ContextMT() override;

    // Context interface
    bool ReadyToRender() const override;
    void OnCommandQueueCompleted(CommandQueue& cmd_queue, uint32_t frame_index) override;
    void WaitForGpu(WaitFor wait_for) override;
    void Resize(const FrameSize& frame_size) override;
    void Present() override;
    bool SetVSyncEnabled(bool vsync_enabled) override;
    Platform::AppView GetAppView() const override { return { m_app_view }; }

    id<CAMetalDrawable>      GetNativeDrawable()       { return m_app_view.currentDrawable; }
    id<MTLDevice>&           GetNativeDevice()         { return m_mtl_device; }

protected:
    id<MTLDevice>           m_mtl_device;
    AppViewMT*              m_app_view;
    dispatch_semaphore_t    m_dispatch_semaphore;
};

} // namespace Graphics
} // namespace Methane
