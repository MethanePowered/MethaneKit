/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/FenceBase.h
Methane fence base implementation.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Fence.h>
#include <Methane/Graphics/ObjectBase.h>

namespace Methane::Graphics
{

class CommandQueueBase;

class FenceBase
    : public Fence
    , public ObjectBase
{
public:
    FenceBase(CommandQueueBase& command_queue);

    // Fence overrides
    void Signal() override;
    void Wait() override;
    void Flush() override;

protected:
    CommandQueueBase& GetCommandQueue() noexcept { return m_command_queue; }
    uint64_t          GetValue() const noexcept  { return m_value; }

private:
    CommandQueueBase& m_command_queue;
    uint64_t          m_value = 0u;
};

} // namespace Methane::Graphics
