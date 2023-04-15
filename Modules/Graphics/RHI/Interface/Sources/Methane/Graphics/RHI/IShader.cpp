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

FILE: Methane/Graphics/RHI/IShader.cpp
Methane shader interface: defines programmable stage of the graphics pipeline.

******************************************************************************/

#include <Methane/Graphics/RHI/IShader.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Instrumentation.h>

#include <sstream>

namespace Methane::Graphics::Rhi
{

ShaderMacroDefinition::ShaderMacroDefinition(std::string name)
    : name(std::move(name))
{ }

ShaderMacroDefinition::ShaderMacroDefinition(std::string name, std::string value)
    : name(std::move(name))
    , value(std::move(value))
{ }

bool ShaderMacroDefinition::operator==(const ShaderMacroDefinition& other) const noexcept
{
    return std::tie(name, value) == std::tie(other.name, other.value);
}

bool ShaderMacroDefinition::operator!=(const ShaderMacroDefinition& other) const noexcept
{
    return std::tie(name, value) != std::tie(other.name, other.value);
}

bool ShaderEntryFunction::operator==(const ShaderEntryFunction& other) const noexcept
{
    return std::tie(file_name, function_name) == std::tie(other.file_name, other.function_name);
}

bool ShaderEntryFunction::operator!=(const ShaderEntryFunction& other) const noexcept
{
    return std::tie(file_name, function_name) != std::tie(other.file_name, other.function_name);
}

bool ShaderSettings::operator==(const ShaderSettings& other) const noexcept
{
    if (std::addressof(data_provider) != std::addressof(other.data_provider))
        return false;

    return std::tie(entry_function, compile_definitions, source_file_path, source_compile_target)
        == std::tie(other.entry_function, other.compile_definitions, other.source_file_path, other.source_compile_target);
}

bool ShaderSettings::operator!=(const ShaderSettings& other) const noexcept
{
    if (std::addressof(data_provider) == std::addressof(other.data_provider))
        return false;

    return std::tie(entry_function, compile_definitions, source_file_path, source_compile_target)
        != std::tie(other.entry_function, other.compile_definitions, other.source_file_path, other.source_compile_target);
}

Ptr<IShader> IShader::Create(Type type, const IContext& context, const Settings& settings)
{
    return context.CreateShader(type, settings);
}

std::string ShaderMacroDefinition::ToString(const std::vector<ShaderMacroDefinition>& macro_definitions, std::string_view splitter) noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    bool is_first_definition = true;
    for(const ShaderMacroDefinition& macro_definition : macro_definitions)
    {
        if (!is_first_definition)
            ss << splitter;

        ss << macro_definition.name << "=" << macro_definition.value;
        is_first_definition = false;
    }
    return ss.str();
}

} // namespace Methane::Graphics::Rhi
