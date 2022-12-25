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

#include "Pimpl.h"

#include <Methane/Graphics/RHI/IShader.h>

namespace Methane::Graphics::Rhi
{

class RenderContext;

class Shader
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

    explicit Shader(const Ptr<IShader>& interface_ptr);
    explicit Shader(IShader& interface_ref);
    Shader(Type type, const RenderContext& context, const Settings& settings);

    void Init(Type type, const RenderContext& context, const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IShader& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IShader> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IShader interface methods
    [[nodiscard]] Type            GetType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    ImplPtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
