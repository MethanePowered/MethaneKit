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

FILE: Methane/Graphics/RHI/Shader.cpp
Methane Shader PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#include <Methane/Graphics/RHI/Shader.h>
#include <Methane/Graphics/RHI/RenderContext.h>

#if defined METHANE_GFX_DIRECTX

#include <Methane/Graphics/DirectX/Shader.h>
using ShaderImpl = Methane::Graphics::DirectX::Shader;

#elif defined METHANE_GFX_VULKAN

#include <Methane/Graphics/Vulkan/Shader.h>
using ShaderImpl = Methane::Graphics::Vulkan::Shader;

#elif defined METHANE_GFX_METAL

#include <Methane/Graphics/Metal/Shader.hh>
using ShaderImpl = Methane::Graphics::Metal::Shader;

#else // METHAN_GFX_[API] is undefined

static_assert(false, "Static graphics API macro-definition is missing.");

#endif

#include "ImplWrapper.hpp"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

class Shader::Impl
    : public ImplWrapper<IShader, ShaderImpl>
{
public:
    using ImplWrapper::ImplWrapper;
};

META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Shader);

Shader::Shader(const Ptr<IShader>& interface_ptr)
    : m_impl_ptr(std::make_unique<Impl>(interface_ptr))
{
}

Shader::Shader(IShader& interface_ref)
    : Shader(std::dynamic_pointer_cast<IShader>(interface_ref.GetPtr()))
{
}

Shader::Shader(Type type, const RenderContext& context, const Settings& settings)
    : Shader(IShader::Create(type, context.GetInterface(), settings))
{
}

void Shader::Init(Type type, const RenderContext& context, const Settings& settings)
{
    m_impl_ptr = std::make_unique<Impl>(IShader::Create(type, context.GetInterface(), settings));
}

void Shader::Release()
{
    m_impl_ptr.reset();
}

bool Shader::IsInitialized() const META_PIMPL_NOEXCEPT
{
    return static_cast<bool>(m_impl_ptr);
}

IShader& Shader::GetInterface() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterface(m_impl_ptr);
}

Ptr<IShader> Shader::GetInterfacePtr() const META_PIMPL_NOEXCEPT
{
    return GetPublicInterfacePtr(m_impl_ptr);
}

ShaderType Shader::GetType() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetType();
}

const ShaderSettings& Shader::GetSettings() const META_PIMPL_NOEXCEPT
{
    return GetPrivateImpl(m_impl_ptr).GetSettings();
}

} // namespace Methane::Graphics::Rhi
