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

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <Fence.hh>
#else
#include <Fence.h>
#endif

namespace Methane::Graphics::Rhi
{

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Fence);
META_PIMPL_METHODS_COMPARE_IMPLEMENT(Fence);

Fence::Fence(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

Fence::Fence(const Ptr<IFence>& interface_ptr)
    : Fence(std::dynamic_pointer_cast<Impl>(interface_ptr))
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
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IFence::Create(command_queue.GetInterface()));
}

void Fence::Release()
{
    m_impl_ptr.reset();
}

bool Fence::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IFence& Fence::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IFence> Fence::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Fence::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Fence::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void Fence::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void Fence::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

void Fence::Signal() const
{
    GetImpl(m_impl_ptr).Signal();
}

void Fence::WaitOnCpu() const
{
    GetImpl(m_impl_ptr).WaitOnCpu();
}

void Fence::WaitOnGpu(ICommandQueue& wait_on_command_queue) const
{
    GetImpl(m_impl_ptr).WaitOnGpu(wait_on_command_queue);
}

void Fence::FlushOnCpu() const
{
    GetImpl(m_impl_ptr).FlushOnCpu();
}

void Fence::FlushOnGpu(ICommandQueue& wait_on_command_queue) const
{
    GetImpl(m_impl_ptr).FlushOnGpu(wait_on_command_queue);
}

} // namespace Methane::Graphics::Rhi
