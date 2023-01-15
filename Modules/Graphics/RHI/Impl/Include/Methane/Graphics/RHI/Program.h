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

namespace Methane::Graphics::META_GFX_NAME
{
class Program;
}

namespace Methane::Graphics::Rhi
{

struct ProgramSettingsImpl
{
    using ShaderSet = std::map<ShaderType, ShaderSettings>;

    ShaderSet                 shader_set;
    ProgramInputBufferLayouts input_buffer_layouts;
    ProgramArgumentAccessors  argument_accessors;
    AttachmentFormats         attachment_formats;

    META_RHI_API static ProgramSettings Convert(const IContext& context, const ProgramSettingsImpl& settings);
};

class RenderContext;
class Shader;
class ProgramBindings;

class Program // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Settings                = ProgramSettingsImpl;
    using ShaderSet               = ProgramSettingsImpl::ShaderSet;
    using InputBufferLayout       = ProgramInputBufferLayout;
    using InputBufferLayouts      = ProgramInputBufferLayouts;
    using Argument                = ProgramArgument;
    using Arguments               = ProgramArguments;
    using ArgumentAccessor        = ProgramArgumentAccessor;
    using ArgumentAccessors       = ProgramArgumentAccessors;
    using ResourceViewsByArgument = IProgram::ResourceViewsByArgument;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Program);
    META_PIMPL_METHODS_COMPARE_DECLARE(Program);

    META_RHI_API explicit Program(const Ptr<IProgram>& interface_ptr);
    META_RHI_API explicit Program(IProgram& interface_ref);
    META_RHI_API Program(const RenderContext& context, const Settings& settings);

    META_RHI_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_RHI_API IProgram& GetInterface() const META_PIMPL_NOEXCEPT;
    META_RHI_API Ptr<IProgram> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_RHI_API bool SetName(std::string_view name) const;
    META_RHI_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_RHI_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_RHI_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IProgram interface methods
    [[nodiscard]] META_RHI_API ProgramBindings        CreateBindings(const ResourceViewsByArgument& resource_views_by_argument, Data::Index frame_index = 0U) const;
    [[nodiscard]] META_RHI_API const ProgramSettings& GetSettings() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API const ShaderTypes&     GetShaderTypes() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_RHI_API Shader                 GetShader(ShaderType shader_type) const;
    [[nodiscard]] META_RHI_API Data::Size             GetBindingsCount() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Program;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_RHI_PIMPL_INLINE

#include <Methane/Graphics/RHI/Program.cpp>

#endif // META_RHI_PIMPL_INLINE
