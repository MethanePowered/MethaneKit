/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ComputeContext.cpp
Methane ComputeContext PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/ComputeContext.h>
#include <Methane/Graphics/RHI/Device.h>
#include <Methane/Graphics/RHI/CommandKit.h>
#include <Methane/Graphics/RHI/CommandQueue.h>
#include <Methane/Graphics/RHI/Shader.h>
#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/Buffer.h>
#include <Methane/Graphics/RHI/Texture.h>
#include <Methane/Graphics/RHI/Sampler.h>

#include <Methane/Pimpl.hpp>

#ifdef META_GFX_METAL
#include <ComputeContext.hh>
#else
#include <ComputeContext.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(ComputeContext);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(ComputeContext);

ComputeContext::ComputeContext(const Ptr<IComputeContext>& interface_ptr)
    : m_impl_ptr(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

ComputeContext::ComputeContext(IComputeContext& render_context)
    : ComputeContext(render_context.GetDerivedPtr<IComputeContext>())
{
}

ComputeContext::ComputeContext(const Device& device, tf::Executor& parallel_executor, const Settings& settings)
    : ComputeContext(IComputeContext::Create(device.GetInterface(), parallel_executor, settings))
{
}

bool ComputeContext::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IComputeContext& ComputeContext::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IComputeContext> ComputeContext::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool ComputeContext::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view ComputeContext::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void ComputeContext::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void ComputeContext::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

CommandQueue ComputeContext::CreateCommandQueue(CommandListType type) const
{
    return CommandQueue(GetImpl(m_impl_ptr).CreateCommandQueue(type));
}

CommandKit ComputeContext::CreateCommandKit(CommandListType type) const
{
    return CommandKit(GetImpl(m_impl_ptr).CreateCommandKit(type));
}

Shader ComputeContext::CreateShader(ShaderType type, const ShaderSettings& settings) const
{
    return Shader(GetImpl(m_impl_ptr).CreateShader(type, settings));
}

Program ComputeContext::CreateProgram(const ProgramSettingsImpl& settings) const
{
    return Program(GetImpl(m_impl_ptr).CreateProgram(ProgramSettingsImpl::Convert(GetInterface(), settings)));
}

Buffer ComputeContext::CreateBuffer(const BufferSettings& settings) const
{
    return Buffer(GetImpl(m_impl_ptr).CreateBuffer(settings));
}

Texture ComputeContext::CreateTexture(const TextureSettings& settings) const
{
    return Texture(GetImpl(m_impl_ptr).CreateTexture(settings));
}

Sampler ComputeContext::CreateSampler(const SamplerSettings& settings) const
{
    return Sampler(GetImpl(m_impl_ptr).CreateSampler(settings));
}

ContextOptionMask ComputeContext::GetOptions() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetOptions();
}

tf::Executor& ComputeContext::GetParallelExecutor() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetParallelExecutor();
}

IObjectRegistry& ComputeContext::GetObjectRegistry() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetObjectRegistry();
}

void ComputeContext::RequestDeferredAction(DeferredAction action) const META_PIMPL_NOEXCEPT
{
    GetImpl(m_impl_ptr).RequestDeferredAction(action);
}

void ComputeContext::CompleteInitialization() const
{
    GetImpl(m_impl_ptr).CompleteInitialization();
}

bool ComputeContext::IsCompletingInitialization() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).IsCompletingInitialization();
}

void ComputeContext::WaitForGpu(WaitFor wait_for) const
{
    GetImpl(m_impl_ptr).WaitForGpu(wait_for);
}

void ComputeContext::Reset(const Device& device) const
{
    GetImpl(m_impl_ptr).Reset(device.GetInterface());
}

void ComputeContext::Reset() const
{
    GetImpl(m_impl_ptr).Reset();
}

Device ComputeContext::GetDevice() const
{
    return Device(const_cast<IDevice&>(GetImpl(m_impl_ptr).GetDevice()));
}

CommandKit ComputeContext::GetDefaultCommandKit(CommandListType type) const
{
    return CommandKit(GetImpl(m_impl_ptr).GetDefaultCommandKit(type));
}

CommandKit ComputeContext::GetDefaultCommandKit(const CommandQueue& cmd_queue) const
{
    return CommandKit(GetImpl(m_impl_ptr).GetDefaultCommandKit(cmd_queue.GetInterface()));
}

CommandKit ComputeContext::GetUploadCommandKit() const
{
    return CommandKit(GetImpl(m_impl_ptr).GetUploadCommandKit());
}

CommandKit ComputeContext::GetComputeCommandKit() const
{
    return CommandKit(GetImpl(m_impl_ptr).GetComputeCommandKit());
}

void ComputeContext::Connect(Data::Receiver<IContextCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IContextCallback>::Connect(receiver);
}

void ComputeContext::Disconnect(Data::Receiver<IContextCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IContextCallback>::Disconnect(receiver);
}

const ComputeContextSettings& ComputeContext::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
