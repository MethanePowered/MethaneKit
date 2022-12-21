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
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class RenderContext;
class Shader;

class Program
    : public Data::Transmitter<Rhi::IObjectCallback>
{
public:
    using Shaders            = std::vector<Shader>;
    using InputBufferLayout  = ProgramInputBufferLayout;
    using InputBufferLayouts = ProgramInputBufferLayouts;
    using Argument           = ProgramArgument;
    using Arguments          = ProgramArguments;
    using ArgumentAccessor   = ProgramArgumentAccessor;
    using ArgumentAccessors  = ProgramArgumentAccessors;

    struct Settings
    {
        std::vector<Shader>       shaders;
        ProgramInputBufferLayouts input_buffer_layouts;
        ProgramArgumentAccessors  argument_accessors;
        AttachmentFormats         attachment_formats;
    };

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
    [[nodiscard]] const ProgramSettings& GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const ShaderTypes&     GetShaderTypes() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] Shader                 GetShader(ShaderType shader_type) const;
    [[nodiscard]] Data::Size             GetBindingsCount() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    Program(UniquePtr<Impl>&& impl_ptr);

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
