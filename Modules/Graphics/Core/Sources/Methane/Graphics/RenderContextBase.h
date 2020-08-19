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

FILE: Methane/Graphics/RenderContextBase.h
Base implementation of the render context interface.

******************************************************************************/

#pragma once

#include "ContextBase.h"
#include "FenceBase.h"

#include <Methane/Graphics/RenderContext.h>
#include <Methane/Graphics/FpsCounter.h>

namespace Methane::Graphics
{

class RenderContextBase
    : public ContextBase
    , public RenderContext
{
public:
    RenderContextBase(DeviceBase& device, tf::Executor& parallel_executor, const Settings& settings);

    // Context interface
    void WaitForGpu(WaitFor wait_for) override;

    // RenderContext interface
    void              Resize(const FrameSize& frame_size) override;
    void              Present() override;
    CommandQueue&     GetRenderCommandQueue() override;
    const Settings&   GetSettings() const noexcept final            { return m_settings; }
    uint32_t          GetFrameBufferIndex() const noexcept final    { return m_frame_buffer_index;  }
    uint32_t          GetFrameIndex() const noexcept final          { return m_frame_index; }
    const FpsCounter& GetFpsCounter() const noexcept final          { return m_fps_counter; }
    bool              SetVSyncEnabled(bool vsync_enabled) override;
    bool              SetFrameBuffersCount(uint32_t frame_buffers_count) override;
    bool              SetFullScreen(bool is_full_screen) override;

    // ContextBase interface
    void Initialize(DeviceBase& device, bool deferred_heap_allocation, bool is_callback_emitted = true) override;
    void Release() override;

    // Object interface
    void SetName(const std::string& name) override;

    // Frame buffer is in use while there are executing rendering commands contributing to this frame buffer
    bool IsFrameBufferInUse() const noexcept { return m_is_frame_buffer_in_use; }

protected:
    void ResetWithSettings(const Settings& settings);
    void OnCpuPresentComplete(bool signal_frame_fence = true);
    void UpdateFrameBufferIndex();

    inline const Ptr<Fence>& GetCurrentFrameFencePtr() const { return m_frame_fences[m_frame_buffer_index]; }
    Fence&                   GetCurrentFrameFence() const;
    Fence&                   GetRenderFence() const;

    // ContextBase overrides
    bool UploadResources() override;
    void OnGpuWaitStart(WaitFor wait_for) override;
    void OnGpuWaitComplete(WaitFor wait_for) override;

    // RenderContextBase
    virtual uint32_t GetNextFrameBufferIndex();

private:
    Settings            m_settings;
    Ptr<CommandQueue>   m_sp_render_cmd_queue;
    Ptrs<Fence>         m_frame_fences;
    Ptr<Fence>          m_sp_render_fence;
    uint32_t            m_frame_buffer_index = 0u;
    uint32_t            m_frame_index = 0u;
    bool                m_is_frame_buffer_in_use = true;
    FpsCounter          m_fps_counter;
};

} // namespace Methane::Graphics
