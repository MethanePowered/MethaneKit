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

FILE: Methane/Graphics/ProgramBase.h
Base implementation of the program interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Program.h>
#include <Methane/Instrumentation.h>

#include "ShaderBase.h"
#include "ProgramBindingsBase.h"

#include <magic_enum.hpp>
#include <memory>
#include <array>
#include <optional>
#include <mutex>

namespace Methane::Graphics
{

class ContextBase;
class CommandListBase;

class ProgramBase
    : public Program
    , public ObjectBase
{
    friend class ShaderBase;
    friend class ProgramBindingsBase;

public:
    ProgramBase(const ContextBase& context, const Settings& settings);

    // Program interface
    const Settings&     GetSettings() const noexcept final            { return m_settings; }
    const ShaderTypes&  GetShaderTypes() const noexcept final         { return m_shader_types; }
    const Ptr<IShader>& GetShader(ShaderType shader_type) const final { return m_shaders_by_type[magic_enum::enum_index(shader_type).value()]; }
    bool                HasShader(ShaderType shader_type) const       { return !!GetShader(shader_type); }
    Data::Size          GetBindingsCount() const noexcept final       { return m_bindings_count; }

    const ContextBase&  GetContext() const { return m_context; }

protected:
    using ArgumentBindings      = ProgramBindingsBase::ArgumentBindings;
    using ArgumentBindingBase   = ProgramBindingsBase::ArgumentBindingBase;
    using FrameArgumentBindings = std::unordered_map<Program::Argument, Ptrs<ProgramBindingsBase::ArgumentBindingBase>, Program::Argument::Hash>;

    void InitArgumentBindings(const ArgumentAccessors& argument_accessors);
    const ArgumentBindings&         GetArgumentBindings() const noexcept      { return m_binding_by_argument; }
    const FrameArgumentBindings&    GetFrameArgumentBindings() const noexcept { return m_frame_bindings_by_argument; }
    const Ptr<ArgumentBindingBase>& GetFrameArgumentBinding(Data::Index frame_index, const Program::ArgumentAccessor& argument_accessor) const;
    Ptr<ArgumentBindingBase>        CreateArgumentBindingInstance(const Ptr<ArgumentBindingBase>& argument_binding_ptr, Data::Index frame_index) const;

    IShader& GetShaderRef(ShaderType shader_type) const;
    uint32_t GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const;

    using ShadersByType = std::array<Ptr<IShader>, magic_enum::enum_count<ShaderType>() - 1>;
    static ShadersByType CreateShadersByType(const Ptrs<IShader>& shaders);

    Data::Size GetBindingsCountAndIncrement() noexcept { return m_bindings_count++; }

private:
    const ContextBase&                    m_context;
    const Settings                        m_settings;
    const ShadersByType                   m_shaders_by_type;
    const ShaderTypes                     m_shader_types;
    ProgramBindingsBase::ArgumentBindings m_binding_by_argument;
    FrameArgumentBindings                 m_frame_bindings_by_argument;
    Data::Size                            m_bindings_count = 0u;
};

} // namespace Methane::Graphics
