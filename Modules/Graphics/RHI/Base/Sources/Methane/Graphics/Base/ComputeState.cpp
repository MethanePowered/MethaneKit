/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/ComputeState.cpp
Base implementation of the compute state interface.

******************************************************************************/

#include <Methane/Graphics/Base/ComputeState.h>
#include <Methane/Graphics/Rhi/IProgram.h>

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

ComputeState::ComputeState(const Rhi::IContext& context, const Settings& settings)
    : m_context(context)
    , m_settings(settings)
{ }

void ComputeState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(settings.program_ptr, "program is not initialized in render state settings");
    META_CHECK_ARG_NOT_NULL_DESCR(settings.program_ptr->GetShader(Rhi::ShaderType::Compute), "Program used in compute state must include compute shader");

    m_settings = settings;
}

Rhi::IProgram& ComputeState::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_settings.program_ptr);
    return *m_settings.program_ptr;
}

} // namespace Methane::Graphics::Base
