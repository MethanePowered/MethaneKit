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
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::METHANE_GFX_API
{
class Fence;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;

class Fence
    : Data::Transmitter<IObjectCallback>
{
public:
    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Fence);
    META_PIMPL_METHODS_COMPARE_DECLARE(Fence);

    explicit Fence(const Ptr<IFence>& interface_ptr);
    explicit Fence(IFence& interface_ref);
    explicit Fence(const CommandQueue& command_queue);

    void Init(const CommandQueue& command_queue);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IFence& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IFence> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // IFence interface methods
    void Signal() const;
    void WaitOnCpu() const;
    void WaitOnGpu(ICommandQueue& wait_on_command_queue) const;
    void FlushOnCpu() const;
    void FlushOnGpu(ICommandQueue& wait_on_command_queue) const;

private:
    using Impl = Methane::Graphics::METHANE_GFX_API::Fence;

    Fence(Ptr<Impl>&& impl_ptr);

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
