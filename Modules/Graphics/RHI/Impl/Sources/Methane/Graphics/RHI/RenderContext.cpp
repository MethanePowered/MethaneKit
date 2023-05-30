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
#include <Methane/Graphics/RHI/Shader.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>
#include <Methane/Graphics/RHI/RenderState.h>
#include <Methane/Graphics/RHI/RenderPattern.h>
#include <Methane/Graphics/RHI/ComputeState.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <RenderContext.hh>
#else
#include <RenderContext.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(RenderContext);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(RenderContext);

RenderContext::RenderContext(const Ptr<IRenderContext>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
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

bool RenderContext::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IRenderContext& RenderContext::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IRenderContext> RenderContext::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool RenderContext::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view RenderContext::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void RenderContext::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void RenderContext::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

CommandQueue RenderContext::CreateCommandQueue(CommandListType type) const
{
    return CommandQueue(GetImpl(m_impl_ptr).CreateCommandQueue(type));
}

CommandKit RenderContext::CreateCommandKit(CommandListType type) const
{
    return CommandKit(GetImpl(m_impl_ptr).CreateCommandKit(type));
}

Shader RenderContext::CreateShader(ShaderType type, const ShaderSettings& settings) const
{
    return Shader(GetImpl(m_impl_ptr).CreateShader(type, settings));
}

Program RenderContext::CreateProgram(const ProgramSettingsImpl& settings) const
{
    return Program(GetImpl(m_impl_ptr).CreateProgram(ProgramSettingsImpl::Convert(GetInterface(), settings)));
}

Buffer RenderContext::CreateBuffer(const BufferSettings& settings) const
{
    return Buffer(GetImpl(m_impl_ptr).CreateBuffer(settings));
}

Texture RenderContext::CreateTexture(const TextureSettings& settings) const
{
    return Texture(GetImpl(m_impl_ptr).CreateTexture(settings));
}

Sampler RenderContext::CreateSampler(const SamplerSettings& settings) const
{
    return Sampler(GetImpl(m_impl_ptr).CreateSampler(settings));
}

RenderState RenderContext::CreateRenderState(const RenderStateSettingsImpl& settings) const
{
    return RenderState(GetImpl(m_impl_ptr).CreateRenderState(RenderStateSettingsImpl::Convert(settings)));
}

ComputeState RenderContext::CreateComputeState(const ComputeStateSettingsImpl& settings) const
{
    return ComputeState(GetImpl(m_impl_ptr).CreateComputeState(ComputeStateSettingsImpl::Convert(settings)));
}

RenderPattern RenderContext::CreateRenderPattern(const RenderPatternSettings& settings) const
{
    return RenderPattern(GetImpl(m_impl_ptr).CreateRenderPattern(settings));
}

ContextOptionMask RenderContext::GetOptions() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetOptions();
}

tf::Executor& RenderContext::GetParallelExecutor() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetParallelExecutor();
}

IObjectRegistry& RenderContext::GetObjectRegistry() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetObjectRegistry();
}

bool RenderContext::UploadResources() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).UploadResources();
}

void RenderContext::RequestDeferredAction(DeferredAction action) const META_PIMPL_NOEXCEPT
{
    GetImpl(m_impl_ptr).RequestDeferredAction(action);
}

void RenderContext::CompleteInitialization() const
{
    GetImpl(m_impl_ptr).CompleteInitialization();
}

bool RenderContext::IsCompletingInitialization() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).IsCompletingInitialization();
}

void RenderContext::WaitForGpu(WaitFor wait_for) const
{
    GetImpl(m_impl_ptr).WaitForGpu(wait_for);
}

void RenderContext::Reset(const Device& device) const
{
    GetImpl(m_impl_ptr).Reset(device.GetInterface());
}

void RenderContext::Reset() const
{
    GetImpl(m_impl_ptr).Reset();
}

Device RenderContext::GetDevice() const
{
    return Device(const_cast<IDevice&>(GetImpl(m_impl_ptr).GetDevice()));
}

CommandKit RenderContext::GetDefaultCommandKit(CommandListType type) const
{
    return CommandKit(GetImpl(m_impl_ptr).GetDefaultCommandKit(type));
}

CommandKit RenderContext::GetDefaultCommandKit(const CommandQueue& cmd_queue) const
{
    return CommandKit(GetImpl(m_impl_ptr).GetDefaultCommandKit(cmd_queue.GetInterface()));
}

CommandKit RenderContext::GetUploadCommandKit() const
{
    return CommandKit(GetImpl(m_impl_ptr).GetUploadCommandKit());
}

CommandKit RenderContext::GetRenderCommandKit() const
{
    return CommandKit(GetImpl(m_impl_ptr).GetRenderCommandKit());
}

void RenderContext::Connect(Data::Receiver<IContextCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IContextCallback>::Connect(receiver);
}

void RenderContext::Disconnect(Data::Receiver<IContextCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IContextCallback>::Disconnect(receiver);
}

bool RenderContext::ReadyToRender() const
{
    return GetImpl(m_impl_ptr).ReadyToRender();
}

void RenderContext::Resize(const FrameSize& frame_size) const
{
    GetImpl(m_impl_ptr).Resize(frame_size);
}

void RenderContext::Present() const
{
    GetImpl(m_impl_ptr).Present();
}

Platform::AppView RenderContext::GetAppView() const
{
    return GetImpl(m_impl_ptr).GetAppView();
}

const RenderContextSettings& RenderContext::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

uint32_t RenderContext::GetFrameBufferIndex() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFrameBufferIndex();
}

uint32_t RenderContext::GetFrameIndex() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFrameIndex();
}

const Data::IFpsCounter& RenderContext::GetFpsCounter() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetFpsCounter();
}

bool RenderContext::SetVSyncEnabled(bool vsync_enabled) const
{
    return GetImpl(m_impl_ptr).SetVSyncEnabled(vsync_enabled);
}

bool RenderContext::SetFrameBuffersCount(uint32_t frame_buffers_count) const
{
    return GetImpl(m_impl_ptr).SetFrameBuffersCount(frame_buffers_count);
}

bool RenderContext::SetFullScreen(bool is_full_screen) const
{
    return GetImpl(m_impl_ptr).SetFullScreen(is_full_screen);
}

} // namespace Methane::Graphics::Rhi
