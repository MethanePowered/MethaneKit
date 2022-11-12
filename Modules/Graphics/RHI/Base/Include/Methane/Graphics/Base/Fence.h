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

FILE: Methane/Graphics/Base/Fence.h
Methane fence base implementation.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/IFence.h>

namespace Methane::Graphics::Base
{

class CommandQueue;

class Fence
    : public IFence
    , public Object
{
public:
    explicit Fence(CommandQueue& command_queue);

    // IFence overrides
    void Signal() override;
    void WaitOnCpu() override;
    void WaitOnGpu(ICommandQueue& wait_on_command_queue) override;
    void FlushOnCpu() override;
    void FlushOnGpu(ICommandQueue& wait_on_command_queue) override;

protected:
    CommandQueue& GetCommandQueue() noexcept { return m_command_queue; }
    uint64_t          GetValue() const noexcept  { return m_value; }

private:
    CommandQueue& m_command_queue;
    uint64_t      m_value = 0U;
};

} // namespace Methane::Graphics::Base
