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

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/Program.h>
using ProgramImpl = Methane::Graphics::DirectX::Program;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Program.h>
using ProgramImpl = Methane::Graphics::Vulkan::Program;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Program.hh>
using ProgramImpl = Methane::Graphics::Metal::Program;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

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

static IProgram::Settings ConvertProgramSettings(const IContext& context, const Program::Settings& settings)
{
    META_FUNCTION_TASK();
    return IProgram::Settings
    {
        ConvertProgramShaderSet(context, settings.shader_set),
        settings.input_buffer_layouts,
        settings.argument_accessors,
        settings.attachment_formats
    };
}

class Program::Impl
    : public ImplWrapper<IProgram, ProgramImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Program);

Program::Program(UniquePtr<Impl>&& impl_ptr)
    : Transmitter(impl_ptr->GetInterface())
    , m_impl_ptr(std::move(impl_ptr))
{
}

Program::Program(const Ptr<IProgram>& interface_ptr)
    : Program(std::make_unique<Impl>(interface_ptr))
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
    m_impl_ptr = std::make_unique<Impl>(IProgram::Create(context.GetInterface(), ConvertProgramSettings(context.GetInterface(), settings)));
    Transmitter::Reset(&m_impl_ptr->GetInterface());
}

void Program::Release()
{
    Transmitter::Reset();
    m_impl_ptr.release();
}

bool Program::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IProgram& Program::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IProgram> Program::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

bool Program::SetName(std::string_view name) const
{
    return GetPrivateImpl(m_impl_ptr).SetName(name);
}

std::string_view Program::GetName() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetName();
}

const ProgramSettings& Program::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

const ShaderTypes& Program::GetShaderTypes() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetShaderTypes();
}

Shader Program::GetShader(ShaderType shader_type) const
{
    return Shader(GetPrivateImpl(m_impl_ptr).GetShader(shader_type));
}

Data::Size Program::GetBindingsCount() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetBindingsCount();
}

} // namespace Methane::Graphics::Rhi
