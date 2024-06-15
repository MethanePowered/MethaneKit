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

FILE: Methane/Graphics/Null/Program.h
Null implementation of the program interface.

******************************************************************************/

#include <Methane/Graphics/Null/Program.h>
#include <Methane/Graphics/Null/ProgramBindings.h>
#include <Methane/Graphics/Base/Context.h>

namespace Methane::Graphics::Null
{

Program::Program(const Base::Context& context, const Settings& settings)
    : Base::Program(context, settings)
{
}

Ptr<Rhi::IProgramBindings> Program::CreateBindings(const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index)
{
    return std::make_shared<ProgramBindings>(*this, binding_value_by_argument, frame_index);
}

void Program::SetArgumentBindings(const ResourceArgumentDescs& argument_descriptions)
{
    for(Rhi::ShaderType shader_type : GetShaderTypes())
    {
        dynamic_cast<Shader&>(GetShaderRef(shader_type)).InitArgumentBindings(argument_descriptions);
    }
    Base::Program::InitArgumentBindings();
}

} // namespace Methane::Graphics::Null
