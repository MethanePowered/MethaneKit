/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/RenderContext.h
Methane RenderContext PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IRenderContext.h>

namespace Methane::Graphics::META_GFX_NAME
{
class RenderContext;
}

namespace Methane::Graphics::Rhi
{

class Device;
class CommandKit;
class CommandQueue;
class ObjectRegistry;

class RenderContext
{
public:
    using Settings              = RenderContextSettings;
    using Type                  = ContextType;
    using WaitFor               = ContextWaitFor;
    using DeferredAction        = ContextDeferredAction;
    using Option                = ContextOption;
    using OptionMask            = ContextOptionMask;
    using IncompatibleException = ContextIncompatibleException;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderContext);
    META_PIMPL_METHODS_COMPARE_DECLARE(RenderContext);

    META_RHI_API explicit RenderContext(const Ptr<IRenderContext>& render_context_ptr);
    META_RHI_API explicit RenderContext(IRenderContext& render_context);
    META_RHI_API RenderContext(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings);

    META_RHI_API void Init(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings);
    META_RHI_API void Release();

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IRenderContext& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IRenderContext> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IContext interface methods
    [[nodiscard]] META_RHI_API OptionMask GetOptions() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API tf::Executor& GetParallelExecutor() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API IObjectRegistry& GetObjectRegistry() const META_PIMPL_NOEXCEPT;
    META_RHI_API void RequestDeferredAction(DeferredAction action) const META_PIMPL_NOEXCEPT;
    META_RHI_API void CompleteInitialization() const;
    [[nodiscard]] META_RHI_API bool IsCompletingInitialization() const META_PIMPL_NOEXCEPT;
    META_RHI_API void WaitForGpu(WaitFor wait_for) const;
    META_RHI_API void Reset(const Device& device) const;
    META_RHI_API void Reset() const;
    [[nodiscard]] META_RHI_API Device GetDevice() const;
    [[nodiscard]] META_RHI_API CommandKit GetDefaultCommandKit(CommandListType type) const;
    [[nodiscard]] META_RHI_API CommandKit GetDefaultCommandKit(const CommandQueue& cmd_queue) const;
    [[nodiscard]] META_RHI_API CommandKit GetUploadCommandKit() const;
    [[nodiscard]] META_RHI_API CommandKit GetRenderCommandKit() const;

    // Data::IEmitter<IContextCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IContextCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IContextCallback>& receiver) const;

    // IRenderContext interface methods
    [[nodiscard]] META_RHI_API bool ReadyToRender() const;
    META_RHI_API void Resize(const FrameSize& frame_size) const;
    META_RHI_API void Present() const;
    [[nodiscard]] META_RHI_API Platform::AppView  GetAppView() const;
    [[nodiscard]] META_RHI_API const Settings&    GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API uint32_t           GetFrameBufferIndex() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API uint32_t           GetFrameIndex() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const IFpsCounter& GetFpsCounter() const META_PIMPL_NOEXCEPT;
    META_RHI_API bool SetVSyncEnabled(bool vsync_enabled) const;
    META_RHI_API bool SetFrameBuffersCount(uint32_t frame_buffers_count) const;
    META_RHI_API bool SetFullScreen(bool is_full_screen) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::RenderContext;

    META_RHI_API RenderContext(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/RenderContext.cpp>

#endif // META_RHI_PIMPL_INLINE
