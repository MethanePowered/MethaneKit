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

FILE: Methane/Graphics/Base/Shader.h
Base implementation of the shader interface.

******************************************************************************/

#pragma once

#include "CommandList.h"
#include "ProgramBindings.h"

#include <Methane/Graphics/IShader.h>

#include <set>
#include <string_view>

namespace Methane::Graphics
{

struct IProgram;

} // namespace Methane::Graphics

namespace Methane::Graphics::Base
{

class Context;
class Program;

class Shader
    : public IShader
    , public std::enable_shared_from_this<Shader>
{
public:
    Shader(Type type, const Context& context, const Settings& settings);

    // IShader interface
    Type             GetType() const noexcept final     { return m_type; }
    const Settings&  GetSettings() const noexcept final { return m_settings; }

    // Shader interface
    using ArgumentBindings = Ptrs<ProgramBindings::ArgumentBinding>;
    virtual ArgumentBindings GetArgumentBindings(const ProgramArgumentAccessors& argument_accessors) const = 0;

    const Context& GetContext() const noexcept { return m_context; }
    std::string_view   GetCachedArgName(std::string_view arg_name) const;
    Ptr<Shader>    GetPtr() { return shared_from_this(); }

protected:
    uint32_t            GetProgramInputBufferIndexByArgumentSemantic(const Program& program, const std::string& argument_semantic) const;
    std::string         GetCompiledEntryFunctionName() const { return GetCompiledEntryFunctionName(m_settings); }

    static std::string GetCompiledEntryFunctionName(const Settings& settings);

private:
    using ArgNamesSet = std::set<std::string, std::less<>>;

    const Type          m_type;
    const Context&      m_context;
    const Settings      m_settings;
    mutable ArgNamesSet m_cached_arg_names;
};

} // namespace Methane::Graphics::Base
