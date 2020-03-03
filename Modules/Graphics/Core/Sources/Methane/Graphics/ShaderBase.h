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

FILE: Methane/Graphics/ShaderBase.h
Base implementation of the shader interface.

******************************************************************************/

#pragma once

#include "CommandListBase.h"
#include "DescriptorHeap.h"
#include "ProgramBindingsBase.h"

#include <Methane/Graphics/Shader.h>

namespace Methane::Graphics
{

class ContextBase;
class ProgramBase;
struct Program;

class ShaderBase
    : public Shader
    , public std::enable_shared_from_this<ShaderBase>
{
public:
    ShaderBase(Type type, ContextBase& context, const Settings& settings);

    // Shader interface
    Type             GetType() const noexcept override       { return m_type; }
    const Settings&  GetSettings() const noexcept override   { return m_settings; }

    // ShaderBase interface
    using ArgumentBindings = Ptrs<ProgramBindingsBase::ArgumentBindingBase>;
    virtual ArgumentBindings GetArgumentBindings(const Program::ArgumentDescriptions& argument_descriptions) const = 0;

    Ptr<ShaderBase> GetPtr()                     { return shared_from_this(); }
    std::string     GetTypeName() const noexcept { return Shader::GetTypeName(m_type); }

protected:
    ContextBase&        GetContext()        { return m_context; }
    const ContextBase&  GetContext() const  { return m_context; }
    uint32_t            GetProgramInputBufferIndexByArgumentSemantic(const ProgramBase& program, const std::string& argument_semantic) const;
    std::string         GetCompiledEntryFunctionName() const;

private:
    const Type        m_type;
    ContextBase&      m_context;
    const Settings    m_settings;
};

} // namespace Methane::Graphics
