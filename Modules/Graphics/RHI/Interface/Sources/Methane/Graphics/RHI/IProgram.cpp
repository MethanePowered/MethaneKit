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

FILE: Methane/Graphics/RHI/IProgram.cpp
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

const ProgramArgumentAccessor* IProgram::FindArgumentAccessor(const ArgumentAccessors& argument_accessors, const ProgramArgument& argument)
{
    META_FUNCTION_TASK();
    if (const auto arg_access_it = argument_accessors.find(ArgumentAccessor(argument));
        arg_access_it != argument_accessors.end())
        return std::to_address(arg_access_it);

    const Argument all_shaders_argument(ShaderType::All, argument.GetName());
    const auto arg_access_it = argument_accessors.find(ArgumentAccessor(all_shaders_argument));
    return arg_access_it == argument_accessors.end() ? nullptr : std::to_address(arg_access_it);
}

Ptr<IProgram> IProgram::Create(IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return context.CreateProgram(settings);
}

} // namespace Methane::Graphics::Rhi
