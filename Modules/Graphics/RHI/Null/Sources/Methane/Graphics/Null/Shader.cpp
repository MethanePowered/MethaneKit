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

FILE: Methane/Graphics/Null/Shader.cpp
Null implementation of the shader interface.

******************************************************************************/

#include "Methane/Graphics/Base/ProgramArgumentBinding.h"
#include "Methane/Graphics/RHI/IProgram.h"
#include <Methane/Graphics/Null/Shader.h>
#include <Methane/Graphics/Null/ProgramArgumentBinding.h>

#include <Methane/Graphics/Base/Context.h>

namespace Methane::Graphics::Null
{

Ptrs<Base::ProgramArgumentBinding> Shader::GetArgumentBindings(const Rhi::ProgramArgumentAccessors&) const
{
    return m_argument_bindings;
}

void Shader::InitArgumentBindings(const ResourceArgumentDescs& argument_descriptions)
{
    m_argument_bindings.clear();
    m_argument_bindings.reserve(argument_descriptions.size());
    for(const auto& [argument_accessor, argument_desc] : argument_descriptions)
    {
        if (argument_accessor.GetShaderType() != GetType())
            continue;

        auto argument_binding_ptr = std::make_shared<ProgramArgumentBinding>(GetContext(),
            Rhi::ProgramArgumentBindingSettings
            {
                argument_accessor,
                argument_desc.resource_type,
                argument_desc.resource_count
            });

        m_argument_bindings.push_back(std::static_pointer_cast<Base::ProgramArgumentBinding>(argument_binding_ptr));
    }
}

} // namespace Methane::Graphics::Null
