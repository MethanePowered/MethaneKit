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

FILE: Methane/Graphics/Vulkan/ShaderVK.mm
Vulkan implementation of the shader interface.

******************************************************************************/

#include "ShaderVK.h"
#include "ContextVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<Shader> Shader::Create(Shader::Type shader_type, Context& context, const Settings& settings)
{
    ITT_FUNCTION_TASK();
    return std::make_shared<ShaderVK>(shader_type, dynamic_cast<ContextBase&>(context), settings);
}

ShaderVK::ShaderVK(Shader::Type shader_type, ContextBase& context, const Settings& settings)
    : ShaderBase(shader_type, context, settings)
{
    ITT_FUNCTION_TASK();
}

ShaderVK::~ShaderVK()
{
    ITT_FUNCTION_TASK();
}

ShaderBase::ArgumentBindings ShaderVK::GetArgumentBindings(const Program::ArgumentDescriptions&) const
{
    ITT_FUNCTION_TASK();
    ArgumentBindings argument_bindings;
    return argument_bindings;
}

IContextVK& ShaderVK::GetContextVK() noexcept
{
    ITT_FUNCTION_TASK();
    return static_cast<IContextVK&>(GetContext());
}

} // namespace Methane::Graphics
