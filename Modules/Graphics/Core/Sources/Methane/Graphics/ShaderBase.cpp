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

FILE: Methane/Graphics/ShaderBase.cpp
Base implementation of the shader interface.

******************************************************************************/

#include "ShaderBase.h"
#include "ProgramBase.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

std::string Shader::ConvertMacroDefinitionsToString(const MacroDefinitions& macro_definitions, std::string_view splitter) noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    bool is_first_defintion = true;
    for(const MacroDefinition& macro_definition : macro_definitions)
    {
        if (!is_first_defintion)
            ss << splitter;

        ss << macro_definition.name << "=" << macro_definition.value;
        is_first_defintion = false;
    }
    return ss.str();
}

ShaderBase::ShaderBase(Type type, const ContextBase& context, const Settings& settings)
    : m_type(type)
    , m_context(context)
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

uint32_t ShaderBase::GetProgramInputBufferIndexByArgumentSemantic(const ProgramBase& program, const std::string& argument_semantic) const
{
    META_FUNCTION_TASK();
    return program.GetInputBufferIndexByArgumentSemantic(argument_semantic);
}

std::string ShaderBase::GetCompiledEntryFunctionName(const Settings& settings)
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

} // namespace Methane::Graphics
