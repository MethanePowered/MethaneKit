/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Base/Shader.cpp
Base implementation of the shader interface.

******************************************************************************/

#include <Methane/Graphics/Base/Shader.h>
#include <Methane/Graphics/Base/Program.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <sstream>

namespace Methane::Graphics::Base
{

Shader::Shader(Type type, const Context& context, const Settings& settings)
    : m_type(type)
    , m_context(context)
    , m_settings(settings)
{ }

uint32_t Shader::GetProgramInputBufferIndexByArgumentSemantic(const Program& program, const std::string& argument_semantic) const
{
    META_FUNCTION_TASK();
    return program.GetInputBufferIndexByArgumentSemantic(argument_semantic);
}

std::string_view Shader::GetCachedArgName(std::string_view arg_name) const
{
    META_FUNCTION_TASK();
    return *m_cached_arg_names.emplace(arg_name).first;
}

std::string Shader::GetCompiledEntryFunctionName(const Settings& settings)
{
    META_FUNCTION_TASK();
    std::stringstream entry_func_steam;
    entry_func_steam << settings.entry_function.file_name << "_" << settings.entry_function.function_name;
    for (const auto& define_and_value : settings.compile_definitions)
    {
        entry_func_steam << "_" << define_and_value.name << define_and_value.value;
    }
    return entry_func_steam.str();
}

} // namespace Methane::Graphics::Base
