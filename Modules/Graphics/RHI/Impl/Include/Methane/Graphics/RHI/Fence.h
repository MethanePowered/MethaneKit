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

FILE: Methane/Graphics/RHI/Fence.h
Methane Fence PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IFence.h>

namespace Methane::Graphics::META_GFX_NAME
{
class Fence;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;

class Fence // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Interface = IFence;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Fence);
    META_PIMPL_METHODS_COMPARE_INLINE(Fence);

    META_PIMPL_API explicit Fence(const Ptr<IFence>& interface_ptr);
    META_PIMPL_API explicit Fence(IFence& interface_ref);
    META_PIMPL_API explicit Fence(const CommandQueue& command_queue);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IFence& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IFence> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IFence interface methods
    META_PIMPL_API void Signal() const;
    META_PIMPL_API void WaitOnCpu() const;
    META_PIMPL_API void WaitOnGpu(const CommandQueue& wait_on_command_queue) const;
    META_PIMPL_API void FlushOnCpu() const;
    META_PIMPL_API void FlushOnGpu(const CommandQueue& wait_on_command_queue) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Fence;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/Fence.cpp>

#endif // META_PIMPL_INLINE
