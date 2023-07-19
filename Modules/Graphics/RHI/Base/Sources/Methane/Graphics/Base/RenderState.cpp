/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/RenderState.cpp
Base implementation of the render state interface.

******************************************************************************/

#include <Methane/Graphics/Base/RenderState.h>
#include <Methane/Graphics/RHI/IProgram.h>

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

RenderState::RenderState(const RenderContext& context, const Settings& settings, bool is_deferred)
    : m_context(context)
    , m_settings(settings)
    , m_is_deferred(is_deferred)
{ }

void RenderState::Reset(const Settings& settings)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL_DESCR(settings.program_ptr, "program is not initialized in render state settings");
    META_CHECK_ARG_NOT_NULL_DESCR(settings.render_pattern_ptr, "render pass pattern is not initialized in render state settings");
    META_CHECK_ARG_NOT_NULL_DESCR(settings.program_ptr->GetShader(Rhi::ShaderType::Vertex), "Program used in render state must include vertex shader");

    m_settings = settings;
}

Rhi::IProgram& RenderState::GetProgram()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_settings.program_ptr);
    return *m_settings.program_ptr;
}

} // namespace Methane::Graphics::Base
