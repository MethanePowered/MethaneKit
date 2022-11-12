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

FILE: Methane/Graphics/IShader.cpp
Methane shader interface: defines programmable stage of the graphics pipeline.

******************************************************************************/

#include <Methane/Graphics/IShader.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

std::string IShader::ConvertMacroDefinitionsToString(const MacroDefinitions& macro_definitions, std::string_view splitter) noexcept
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

} // namespace Methane::Graphics
