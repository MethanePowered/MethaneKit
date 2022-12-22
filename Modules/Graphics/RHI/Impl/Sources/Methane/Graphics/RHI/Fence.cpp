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

FILE: Methane/Graphics/RHI/Fence.cpp
Methane Fence PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Fence.h>
#include <Methane/Graphics/RHI/CommandQueue.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/Fence.h>
using FenceImpl = Methane::Graphics::DirectX::Fence;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Fence.h>
using FenceImpl = Methane::Graphics::Vulkan::Fence;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Fence.hh>
using FenceImpl = Methane::Graphics::Metal::Fence;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class Fence::Impl
    : public ImplWrapper<IFence, FenceImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Fence);

Fence::Fence(UniquePtr<Impl>&& impl_ptr)
    : Data::Transmitter<IObjectCallback>(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

Fence::Fence(const Ptr<IFence>& interface_ptr)
    : Fence(std::make_unique<Impl>(interface_ptr))
{
}

Fence::Fence(IFence& interface_ref)
    : Fence(interface_ref.GetDerivedPtr<IFence>())
{
}

Fence::Fence(const CommandQueue& command_queue)
    : Fence(IFence::Create(command_queue.GetInterface()))
{
}

void Fence::Init(const CommandQueue& command_queue)
{
    m_impl_ptr = std::make_unique<Impl>(IFence::Create(command_queue.GetInterface()));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void Fence::Release()
{
    Transmitter::Reset();
    m_impl_ptr.release();
}

bool Fence::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IFence& Fence::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

bool Fence::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view Fence::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

void Fence::Signal() const
{
    GetPrivateImpl(m_impl_ptr).Signal();
}

void Fence::WaitOnCpu() const
{
    GetPrivateImpl(m_impl_ptr).WaitOnCpu();
}

void Fence::WaitOnGpu(ICommandQueue& wait_on_command_queue) const
{
    GetPrivateImpl(m_impl_ptr).WaitOnGpu(wait_on_command_queue);
}

void Fence::FlushOnCpu() const
{
    GetPrivateImpl(m_impl_ptr).FlushOnCpu();
}

void Fence::FlushOnGpu(ICommandQueue& wait_on_command_queue) const
{
    GetPrivateImpl(m_impl_ptr).FlushOnGpu(wait_on_command_queue);
}

} // namespace Methane::Graphics::Rhi
