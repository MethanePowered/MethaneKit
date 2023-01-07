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

FILE: Methane/Graphics/RHI/Program.cpp
Methane Program PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Program.h>
#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/Shader.h>

#include "Pimpl.hpp"

#ifdef META_GFX_METAL
#include <Program.hh>
#else
#include <Program.h>
#endif

#include <algorithm>

namespace Methane::Graphics::Rhi
{

static IProgram::Shaders ConvertProgramShaderSet(const IContext& context, const Program::ShaderSet& shader_set)
{
    META_FUNCTION_TASK();
    IProgram::Shaders shader_ptrs;
    std::transform(shader_set.begin(), shader_set.end(), std::back_inserter(shader_ptrs),
                   [&context](const std::pair<ShaderType, ShaderSettings>& shader_type_settings)
                   { return IShader::Create(shader_type_settings.first, context, shader_type_settings.second); });
    return shader_ptrs;
}

static ProgramSettings ConvertProgramSettings(const IContext& context, const Program::Settings& settings)
{
    META_FUNCTION_TASK();
    return ProgramSettings
    {
        ConvertProgramShaderSet(context, settings.shader_set),
        settings.input_buffer_layouts,
        settings.argument_accessors,
        settings.attachment_formats
    };
}

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Program);

Program::Program(Ptr<Impl>&& impl_ptr)
    : m_impl_ptr(std::move(impl_ptr))
{
}

Program::Program(const Ptr<IProgram>& interface_ptr)
    : Program(std::dynamic_pointer_cast<Impl>(interface_ptr))
{
}

Program::Program(IProgram& interface_ref)
    : Program(interface_ref.GetDerivedPtr<IProgram>())
{
}

Program::Program(const RenderContext& context, const Settings& settings)
    : Program(IProgram::Create(context.GetInterface(), ConvertProgramSettings(context.GetInterface(), settings)))
{
}

void Program::Init(const RenderContext& context, const Settings& settings)
{
    m_impl_ptr = std::dynamic_pointer_cast<Impl>(IProgram::Create(context.GetInterface(), ConvertProgramSettings(context.GetInterface(), settings)));
}

void Program::Release()
{
    m_impl_ptr.reset();
}

bool Program::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IProgram& Program::GetInterface() const META_PIMPL_NOEXCEPT
{
    return *m_impl_ptr;
}

Ptr<IProgram> Program::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return m_impl_ptr;
}

bool Program::SetName(std::string_view name) const
{
    return GetImpl(m_impl_ptr).SetName(name);
}

std::string_view Program::GetName() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetName();
}

void Program::Connect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Connect(receiver);
}

void Program::Disconnect(Data::Receiver<IObjectCallback>& receiver) const
{
    GetImpl(m_impl_ptr).Data::Emitter<IObjectCallback>::Disconnect(receiver);
}

const ProgramSettings& Program::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetSettings();
}

const ShaderTypes& Program::GetShaderTypes() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetShaderTypes();
}

Shader Program::GetShader(ShaderType shader_type) const
{
    return Shader(GetImpl(m_impl_ptr).GetShader(shader_type));
}

Data::Size Program::GetBindingsCount() const META_PIMPL_NOEXCEPT
{
    return GetImpl(m_impl_ptr).GetBindingsCount();
}

} // namespace Methane::Graphics::Rhi
