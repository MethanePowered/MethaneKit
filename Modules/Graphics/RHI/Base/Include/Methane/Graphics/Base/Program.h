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

FILE: Methane/Graphics/Base/Program.h
Base implementation of the program interface.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "ProgramBindings.h"

#include <Methane/Graphics/RHI/IProgram.h>
#include <Methane/Instrumentation.h>

#include <memory>
#include <array>
#include <optional>
#include <mutex>

namespace Methane::Graphics::Base
{

class Context;
class CommandList;

class Program
    : public Rhi::IProgram
    , public Object
{
    friend class Shader;
    friend class ProgramBindings;

public:
    Program(const Context& context, const Settings& settings);

    // IProgram interface
    const Settings&          GetSettings() const noexcept final                 { return m_settings; }
    const Rhi::ShaderTypes&  GetShaderTypes() const noexcept final              { return m_shader_types; }
    const Ptr<Rhi::IShader>& GetShader(Rhi::ShaderType shader_type) const final;
    bool                     HasShader(Rhi::ShaderType shader_type) const       { return !!GetShader(shader_type); }
    Data::Size               GetBindingsCount() const noexcept final            { return m_bindings_count; }

    const Context& GetContext() const { return m_context; }

protected:
    using ArgumentBinding       = ProgramBindings::ArgumentBinding;
    using ArgumentBindings      = ProgramBindings::ArgumentBindings;
    using FrameArgumentBindings = std::unordered_map<IProgram::Argument, Ptrs<ArgumentBinding>, IProgram::Argument::Hash>;

    void InitArgumentBindings(const ArgumentAccessors& argument_accessors);
    const ArgumentBindings&         GetArgumentBindings() const noexcept      { return m_binding_by_argument; }
    const FrameArgumentBindings&    GetFrameArgumentBindings() const noexcept { return m_frame_bindings_by_argument; }
    const Ptr<ArgumentBinding>&     GetFrameArgumentBinding(Data::Index frame_index, const Rhi::ProgramArgumentAccessor& argument_accessor) const;
    Ptr<ArgumentBinding>            CreateArgumentBindingInstance(const Ptr<ArgumentBinding>& argument_binding_ptr, Data::Index frame_index) const;

    Rhi::IShader& GetShaderRef(Rhi::ShaderType shader_type) const;
    uint32_t GetInputBufferIndexByArgumentSemantic(const std::string& argument_semantic) const;

    using ShadersByType = std::array<Ptr<Rhi::IShader>, magic_enum::enum_count<Rhi::ShaderType>() - 1>;
    static ShadersByType CreateShadersByType(const Ptrs<Rhi::IShader>& shaders);

    Data::Size GetBindingsCountAndIncrement() noexcept { return m_bindings_count++; }

private:
    const Context&         m_context;
    const Settings         m_settings;
    const ShadersByType    m_shaders_by_type;
    const Rhi::ShaderTypes m_shader_types;
    ArgumentBindings       m_binding_by_argument;
    FrameArgumentBindings  m_frame_bindings_by_argument;
    Data::Size             m_bindings_count = 0u;
};

} // namespace Methane::Graphics::Base
