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

FILE: Methane/Graphics/Base/RenderContext.h
Base implementation of the render context interface.

******************************************************************************/

#pragma once

#include "Context.h"
#include "Fence.h"

#include <Methane/Graphics/RHI/IRenderContext.h>
#include <Methane/Data/FpsCounter.h>

namespace Methane::Graphics::Base
{

class RenderContext
    : public Context
    , public Rhi::IRenderContext
{
public:
    RenderContext(Device& device, UniquePtr<Rhi::IDescriptorManager>&& descriptor_manager_ptr,
                  tf::Executor& parallel_executor, const Settings& settings);

    // IContext interface
    [[nodiscard]] OptionMask GetOptions() const noexcept final { return m_settings.options_mask; }
    void WaitForGpu(WaitFor wait_for) override;

    // IRenderContext interface
    void                     Resize(const FrameSize& frame_size) override;
    void                     Present() override;
    const Settings&          GetSettings() const noexcept final            { return m_settings; }
    uint32_t                 GetFrameBufferIndex() const noexcept final    { return m_frame_buffer_index;  }
    uint32_t                 GetFrameIndex() const noexcept final          { return m_frame_index; }
    const Data::IFpsCounter& GetFpsCounter() const noexcept final          { return m_fps_counter; }
    bool                     SetVSyncEnabled(bool vsync_enabled) override;
    bool                     SetFrameBuffersCount(uint32_t frame_buffers_count) override;
    bool                     SetFullScreen(bool is_full_screen) override;
    bool                     UploadResources() const final;

    // Context interface
    void Initialize(Device& device, bool is_callback_emitted = true) override;

protected:
    void ResetWithSettings(const Settings& settings);
    void OnCpuPresentComplete(bool signal_frame_fence = true);
    void UpdateFrameBufferIndex();

    // Rarely actual frame buffers count in swap-chain may be different from the requested,
    // so it may be changed from RenderContextXX::Initialize() method
    void InvalidateFrameBuffersCount(uint32_t frame_buffers_count);
    void InvalidateFrameBufferIndex(uint32_t frame_buffer_index);

    Rhi::IFence& GetCurrentFrameFence() const;
    Rhi::IFence& GetRenderFence() const;

    // Context overrides
    void OnGpuWaitStart(WaitFor wait_for) override;
    void OnGpuWaitComplete(WaitFor wait_for) override;

    // RenderContext
    virtual uint32_t GetNextFrameBufferIndex();

private:
    void WaitForGpuRenderComplete();
    void WaitForGpuFramePresented();

    Settings         m_settings;
    uint32_t         m_frame_buffer_index = 0U;
    uint32_t         m_frame_index = 0U;
    Data::FpsCounter m_fps_counter;
};

} // namespace Methane::Graphics::Base
