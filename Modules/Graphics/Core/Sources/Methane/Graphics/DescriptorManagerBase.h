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

FILE: Methane/Graphics/DescriptorManagerBase.h
Base descriptor manager implementation.

******************************************************************************/

#pragma once

#include "DescriptorManager.h"

#include <Methane/Memory.hpp>
#include <Tracy.hpp>

#include <mutex>

namespace Methane::Graphics
{

struct ProgramBindings;
class ContextBase;

class DescriptorManagerBase : public DescriptorManager
{
public:
    explicit DescriptorManagerBase(ContextBase& context);

    // DescriptorManager interface
    void AddProgramBindings(ProgramBindings& program_bindings) final;
    void CompleteInitialization() override;
    void Release() override;

protected:
    ContextBase& GetContext() { return m_context; }
    const ContextBase& GetContext() const { return m_context; }

private:
    ContextBase&              m_context;
    WeakPtrs<ProgramBindings> m_program_bindings;
    TracyLockable(std::mutex, m_program_bindings_mutex)
};

} // namespace Methane::Graphics
