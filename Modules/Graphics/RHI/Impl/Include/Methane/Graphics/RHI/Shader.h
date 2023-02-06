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

FILE: Methane/Graphics/RHI/Shader.h
Methane Shader PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IShader.h>

namespace Methane::Graphics::META_GFX_NAME
{
class Shader;
}

namespace Methane::Graphics::Rhi
{

class RenderContext;

class Shader // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Type             = ShaderType;
    using Types            = ShaderTypes;
    using MacroDefinition  = ShaderMacroDefinition;
    using MacroDefinitions = ShaderMacroDefinitions;
    using Settings         = ShaderSettings;
    using EntryFunction    = ShaderEntryFunction;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Shader);
    META_PIMPL_METHODS_COMPARE_DECLARE(Shader);

    META_PIMPL_API explicit Shader(const Ptr<IShader>& interface_ptr);
    META_PIMPL_API explicit Shader(IShader& interface_ref);
    META_PIMPL_API Shader(Type type, const RenderContext& context, const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IShader& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IShader> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IShader interface methods
    [[nodiscard]] META_PIMPL_API Type            GetType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::Shader;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/Shader.cpp>

#endif // META_PIMPL_INLINE
