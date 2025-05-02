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

FILE: Methane/Graphics/RHI/ProgramBindings.h
Methane ProgramBindings PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IProgramBindings.h>

namespace Methane::Graphics::META_GFX_NAME
{
class ProgramBindings;
}

namespace Methane::Graphics::Rhi
{

class Program;

class ProgramBindings // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Interface                 = IProgramBindings;
    using IArgumentBindingCallback  = IProgramArgumentBindingCallback;
    using IArgumentBinding          = IProgramArgumentBinding;
    using ApplyBehavior             = ProgramBindingsApplyBehavior;
    using ApplyBehaviorMask         = ProgramBindingsApplyBehaviorMask;
    using UnboundArgumentsException = ProgramBindingsUnboundArgumentsException;
    using BindingValueByArgument    = IProgramBindings::BindingValueByArgument;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ProgramBindings);
    META_PIMPL_METHODS_COMPARE_INLINE(ProgramBindings);

    META_PIMPL_API explicit ProgramBindings(const Ptr<IProgramBindings>& interface_ptr);
    META_PIMPL_API explicit ProgramBindings(IProgramBindings& interface_ref);
    META_PIMPL_API ProgramBindings(const Program& program, const BindingValueByArgument& binding_value_by_argument, Data::Index frame_index = 0U);
    META_PIMPL_API ProgramBindings(const ProgramBindings& other_program_bindings, const BindingValueByArgument& replace_resource_views_by_argument = {},
                                   const Opt<Data::Index>& frame_index = {});

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IProgramBindings& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IProgramBindings> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IProgramBindings interface methods
    [[nodiscard]] META_PIMPL_API Program                 GetProgram() const;
    [[nodiscard]] META_PIMPL_API IArgumentBinding&       Get(const ProgramArgument& shader_argument) const;
    [[nodiscard]] META_PIMPL_API const ProgramArguments& GetArguments() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Data::Index             GetFrameIndex() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API Data::Index             GetBindingsIndex() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API explicit operator       std::string() const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ProgramBindings;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/ProgramBindings.cpp>

#endif // META_PIMPL_INLINE
