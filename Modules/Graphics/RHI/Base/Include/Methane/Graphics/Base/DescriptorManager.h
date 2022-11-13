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

FILE: Methane/Graphics/Base/DescriptorManager.h
Base descriptor manager implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/RHI/IDescriptorManager.h>

#include <Methane/Memory.hpp>
#include <Tracy.hpp>

#include <mutex>

namespace Methane::Graphics::Rhi
{

struct IProgramBindings;

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Base
{

class Context;

class DescriptorManager
    : public Rhi::IDescriptorManager
{
public:
    explicit DescriptorManager(Context& context, bool is_parallel_bindings_processing_enabled = true);

    // IDescriptorManager interface
    void AddProgramBindings(Rhi::IProgramBindings& program_bindings) final;
    void CompleteInitialization() override;
    void Release() override;

protected:
    Context&       GetContext()       { return m_context; }
    const Context& GetContext() const { return m_context; }

    template<typename BindingsFuncType>
    void ForEachProgramBinding(const BindingsFuncType& bindings_functor)
    {
        std::scoped_lock lock_guard(m_program_bindings_mutex);
        for (const WeakPtr<Rhi::IProgramBindings>& program_bindings_wptr : m_program_bindings)
        {
            const Ptr<Rhi::IProgramBindings> program_bindings_ptr = program_bindings_wptr.lock();
            if (program_bindings_ptr)
                bindings_functor(*program_bindings_ptr);
        }
    }

private:
    Context&                        m_context;
    const bool                      m_is_parallel_bindings_processing_enabled;
    WeakPtrs<Rhi::IProgramBindings> m_program_bindings;
    TracyLockable(std::mutex,       m_program_bindings_mutex)
};

} // namespace Methane::Graphics::Base
