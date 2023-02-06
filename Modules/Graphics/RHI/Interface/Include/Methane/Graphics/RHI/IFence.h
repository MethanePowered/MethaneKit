/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/IFence.h
Methane Fence interface used for CPU-GPU synchronization.

******************************************************************************/

#pragma once

#include "IObject.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics::Rhi
{

struct ICommandQueue;

struct IFence
    : virtual IObject // NOSONAR
{
    [[nodiscard]] static Ptr<IFence> Create(ICommandQueue& command_queue);

    // IFence interface
    virtual void Signal() = 0;
    virtual void WaitOnCpu() = 0;
    virtual void WaitOnGpu(ICommandQueue& wait_on_command_queue) = 0;
    virtual void FlushOnCpu() = 0;
    virtual void FlushOnGpu(ICommandQueue& wait_on_command_queue) = 0;
};

} // namespace Methane::Graphics::Rhi
