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

FILE: Methane/Graphics/RHI/RenderContext.cpp
Methane RenderContext PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/CommandQueue.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/RenderContext.h>
using RenderContextImpl = Methane::Graphics::DirectX::RenderContext;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/RenderContext.h>
using RenderContextImpl = Methane::Graphics::Vulkan::RenderContext;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/RenderContext.hh>
using RenderContextImpl = Methane::Graphics::Metal::RenderContext;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include <Methane/Instrumentation.h>

#include "ImplWrapper.hpp"

namespace Methane::Graphics::Rhi
{

class RenderContext::Impl
    : public ImplWrapper<IRenderContext, RenderContextImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderContext);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(RenderContext);

RenderContext::RenderContext(ImplPtr<Impl>&& impl_ptr)
    : Transmitter<IObjectCallback>(impl_ptr->GetInterface())
    , Transmitter<IContextCallback>(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

RenderContext::RenderContext(const Ptr<IRenderContext>& interface_ptr)
    : RenderContext(std::make_unique<Impl>(interface_ptr))
{
}

RenderContext::RenderContext(IRenderContext& render_context)
    : RenderContext(render_context.GetDerivedPtr<IRenderContext>())
{
}

RenderContext::RenderContext(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings)
    : RenderContext(IRenderContext::Create(env, device.GetInterface(), parallel_executor, settings))
{
}

void RenderContext::Init(const Platform::AppEnvironment& env, const Device& device, tf::Executor& parallel_executor, const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(IRenderContext::Create(env, device.GetInterface(), parallel_executor, settings));
    Transmitter<IObjectCallback>::Reset(&m_impl_ptr->GetInterface());
    Transmitter<IContextCallback>::Reset(&m_impl_ptr->GetInterface());
}

void RenderContext::Release()
{
    Transmitter<IObjectCallback>::Reset();
    Transmitter<IContextCallback>::Reset();
    m_impl_ptr.reset();
}

bool RenderContext::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderContext& RenderContext::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IRenderContext> RenderContext::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

bool RenderContext::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view RenderContext::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

ContextOptionMask RenderContext::GetOptions() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetOptions();
}

tf::Executor& RenderContext::GetParallelExecutor() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetParallelExecutor();
}

IObjectRegistry& RenderContext::GetObjectRegistry() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetObjectRegistry();
}

void RenderContext::RequestDeferredAction(DeferredAction action) const META_PIMPL_NOEXCEPT
{
    GetPrivateImpl(m_impl_ptr).RequestDeferredAction(action);
}

void RenderContext::CompleteInitialization() const
{
    GetPrivateImpl(m_impl_ptr).CompleteInitialization();
}

bool RenderContext::IsCompletingInitialization() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).IsCompletingInitialization();
}

void RenderContext::WaitForGpu(WaitFor wait_for) const
{
    GetPrivateImpl(m_impl_ptr).WaitForGpu(wait_for);
}

void RenderContext::Reset(const Device& device) const
{
    GetPrivateImpl(m_impl_ptr).Reset(device.GetInterface());
}

void RenderContext::Reset() const
{
    GetPrivateImpl(m_impl_ptr).Reset();
}

Device RenderContext::GetDevice() const
{
    return Device(const_cast<IDevice&>(GetPrivateImpl(m_impl_ptr).GetDevice()));
}

CommandKit RenderContext::GetDefaultCommandKit(CommandListType type) const
{
    return CommandKit(GetPrivateImpl(m_impl_ptr).GetDefaultCommandKit(type));
}

CommandKit RenderContext::GetDefaultCommandKit(const CommandQueue& cmd_queue) const
{
    return CommandKit(GetPrivateImpl(m_impl_ptr).GetDefaultCommandKit(cmd_queue.GetInterface()));
}

CommandKit RenderContext::GetUploadCommandKit() const
{
    return CommandKit(GetPrivateImpl(m_impl_ptr).GetUploadCommandKit());
}

CommandKit RenderContext::GetRenderCommandKit() const
{
    return CommandKit(GetPrivateImpl(m_impl_ptr).GetRenderCommandKit());
}

bool RenderContext::ReadyToRender() const
{
    return GetPrivateImpl(m_impl_ptr).ReadyToRender();
}

void RenderContext::Resize(const FrameSize& frame_size) const
{
    GetPrivateImpl(m_impl_ptr).Resize(frame_size);
}

void RenderContext::Present() const
{
    GetPrivateImpl(m_impl_ptr).Present();
}

Platform::AppView RenderContext::GetAppView() const
{
    return GetPrivateImpl(m_impl_ptr).GetAppView();
}

const RenderContextSettings& RenderContext::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

uint32_t RenderContext::GetFrameBufferIndex() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFrameBufferIndex();
}

uint32_t RenderContext::GetFrameIndex() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFrameIndex();
}

const IFpsCounter& RenderContext::GetFpsCounter() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetFpsCounter();
}

bool RenderContext::SetVSyncEnabled(bool vsync_enabled) const
{
    return GetPrivateImpl(m_impl_ptr).SetVSyncEnabled(vsync_enabled);
}

bool RenderContext::SetFrameBuffersCount(uint32_t frame_buffers_count) const
{
    return GetPrivateImpl(m_impl_ptr).SetFrameBuffersCount(frame_buffers_count);
}

bool RenderContext::SetFullScreen(bool is_full_screen) const
{
    return GetPrivateImpl(m_impl_ptr).SetFullScreen(is_full_screen);
}

} // namespace Methane::Graphics::Rhi
