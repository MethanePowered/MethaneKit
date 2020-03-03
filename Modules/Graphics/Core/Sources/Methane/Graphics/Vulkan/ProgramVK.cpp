/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ProgramVK.h
Vulkan implementation of the program interface.

******************************************************************************/

#include "ProgramVK.h"
#include "ShaderVK.h"
#include "BufferVK.h"
#include "ContextVK.h"
#include "RenderCommandListVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<Program> Program::Create(Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ProgramVK>(dynamic_cast<ContextBase&>(context), settings);
}

ProgramVK::ProgramVK(ContextBase& context, const Settings& settings)
    : ProgramBase(context, settings)
{
    ITT_FUNCTION_TASK();
}

ProgramVK::~ProgramVK()
{
    ITT_FUNCTION_TASK();
}

IContextVK& ProgramVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetContext());
}

ShaderVK& ProgramVK::GetShaderVK(Shader::Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<ShaderVK&>(GetShaderRef(shader_type));
}

} // namespace Methane::Graphics
