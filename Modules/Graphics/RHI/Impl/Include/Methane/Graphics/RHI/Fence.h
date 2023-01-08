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

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IFence.h>

namespace Methane::Graphics::META_GFX_NAME
{
class Fence;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;

class Fence
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Fence);
    META_PIMPL_METHODS_COMPARE_DECLARE(Fence);

    META_RHI_API explicit Fence(const Ptr<IFence>& interface_ptr);
    META_RHI_API explicit Fence(IFence& interface_ref);
    META_RHI_API explicit Fence(const CommandQueue& command_queue);

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IFence& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IFence> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IFence interface methods
    META_RHI_API void Signal() const;
    META_RHI_API void WaitOnCpu() const;
    META_RHI_API void WaitOnGpu(ICommandQueue& wait_on_command_queue) const;
    META_RHI_API void FlushOnCpu() const;
    META_RHI_API void FlushOnGpu(ICommandQueue& wait_on_command_queue) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Fence;

    META_RHI_API Fence(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/Fence.cpp>

#endif // META_RHI_PIMPL_INLINE
