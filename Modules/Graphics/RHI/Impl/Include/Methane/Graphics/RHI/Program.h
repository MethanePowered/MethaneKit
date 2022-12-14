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

FILE: Methane/Graphics/RHI/Program.h
Methane Program PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IProgram.h>

namespace Methane::Graphics::Rhi
{

class RenderContext;
class Shader;

class Program
{
public:
    using Shaders            = ProgramShaders;
    using Settings           = ProgramSettings;
    using InputBufferLayout  = ProgramInputBufferLayout;
    using InputBufferLayouts = ProgramInputBufferLayouts;
    using Argument           = ProgramArgument;
    using Arguments          = ProgramArguments;
    using ArgumentAccessor   = ProgramArgumentAccessor;
    using ArgumentAccessors  = ProgramArgumentAccessors;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Program);

    Program(const Ptr<IProgram>& interface_ptr);
    Program(IProgram& interface_ref);
    Program(const RenderContext& context, const Settings& settings);

    void Init(const RenderContext& context, const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IProgram& GetInterface() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // IProgram interface methods
    [[nodiscard]] const Settings&    GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const ShaderTypes& GetShaderTypes() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Shader             GetShader(ShaderType shader_type) const;
    [[nodiscard]] Data::Size         GetBindingsCount() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
