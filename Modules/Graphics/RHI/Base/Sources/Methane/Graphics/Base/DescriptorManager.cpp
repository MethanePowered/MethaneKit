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

FILE: Methane/Graphics/Base/DescriptorManager.cpp
Base descriptor manager implementation.

******************************************************************************/

#include <Methane/Graphics/Base/DescriptorManager.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/ProgramBindings.h>

#include <Methane/Instrumentation.h>

#include <taskflow/algorithm/for_each.hpp>

namespace Methane::Graphics::Base
{

DescriptorManager::DescriptorManager(Context& context, bool is_parallel_bindings_processing_enabled)
    : m_context(context)
    , m_is_parallel_bindings_processing_enabled(is_parallel_bindings_processing_enabled)
{ }

void DescriptorManager::CompleteInitialization()
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_program_bindings_mutex);

    const auto program_bindings_end_it = std::remove_if(m_program_bindings.begin(), m_program_bindings.end(),
        [](const WeakPtr<Rhi::IProgramBindings>& program_bindings_wptr)
        { return program_bindings_wptr.expired(); }
    );

    m_program_bindings.erase(program_bindings_end_it, m_program_bindings.end());

    constexpr auto binding_initialization_completer = [](const WeakPtr<Rhi::IProgramBindings>& program_bindings_wptr)
    {
        META_FUNCTION_TASK();
        // Some binding pointers may become expired here due to command list retained resources cleanup on execution completion
        Ptr<Rhi::IProgramBindings> program_bindings_ptr = program_bindings_wptr.lock();
        if (!program_bindings_ptr)
            return;

        static_cast<ProgramBindings&>(*program_bindings_ptr).CompleteInitialization();
    };

    if (m_is_parallel_bindings_processing_enabled)
    {
        tf::Taskflow task_flow;
        task_flow.for_each(m_program_bindings.begin(), m_program_bindings.end(), binding_initialization_completer);
        m_context.GetParallelExecutor().run(task_flow).get();
    }
    else
    {
        for (const WeakPtr<Rhi::IProgramBindings>& program_bindings_wptr : m_program_bindings)
            binding_initialization_completer(program_bindings_wptr);
    }
}

void DescriptorManager::Release()
{
    META_FUNCTION_TASK();
    m_program_bindings.clear();
}

void DescriptorManager::AddProgramBindings(Rhi::IProgramBindings& program_bindings)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_program_bindings_mutex);

#ifdef _DEBUG
    // This may cause performance drop on adding massive amount of program bindings,
    // so we assume that only different program bindings are added and check it in Debug builds only
    const auto program_bindings_it = std::find_if(m_program_bindings.begin(), m_program_bindings.end(),
        [&program_bindings](const WeakPtr<Rhi::IProgramBindings>& program_bindings_ptr)
        { return !program_bindings_ptr.expired() && program_bindings_ptr.lock().get() == std::addressof(program_bindings); }
    );
    META_CHECK_ARG_DESCR("program_bindings", program_bindings_it == m_program_bindings.end(),
        "program bindings instance was already added to resource manager");
#endif

    m_program_bindings.push_back(static_cast<ProgramBindings&>(program_bindings).GetPtr<ProgramBindings>());
}

} // namespace Methane::Graphics::Base
