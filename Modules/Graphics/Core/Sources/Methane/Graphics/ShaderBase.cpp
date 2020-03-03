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

FILE: Methane/Graphics/ShaderBase.cpp
Base implementation of the shader interface.

******************************************************************************/

#include "ShaderBase.h"
#include "ProgramBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

std::string Shader::GetTypeName(Type shader_type) noexcept
{
    ITT_FUNCTION_TASK();
    switch (shader_type)
    {
    case Type::Vertex:    return "Vertex";
    case Type::Pixel:     return "Pixel";
    case Type::All:       return "All";
    default:              assert(0);
    }
    return "Unknown";
}

ShaderBase::ShaderBase(Type type, ContextBase& context, const Settings& settings)
    : m_type(type)
    , m_context(context)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

uint32_t ShaderBase::GetProgramInputBufferIndexByArgumentSemantic(const ProgramBase& program, const std::string& argument_semantic) const
{
    ITT_FUNCTION_TASK();
    return program.GetInputBufferIndexByArgumentSemantic(argument_semantic);
}

std::string ShaderBase::GetCompiledEntryFunctionName() const
{
    ITT_FUNCTION_TASK();
    std::stringstream entry_func_steam;
    entry_func_steam << m_settings.entry_function.file_name << "_" << m_settings.entry_function.function_name;
    for (const auto& define_and_value : m_settings.compile_definitions)
    {
        entry_func_steam << "_" << define_and_value.first << define_and_value.second;
    }
    return entry_func_steam.str();
}

} // namespace Methane::Graphics
