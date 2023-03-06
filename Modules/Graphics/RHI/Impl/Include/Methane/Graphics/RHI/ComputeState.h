/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ComputeState.h
Methane ComputeState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>
#include "Program.h"

#include <Methane/Graphics/RHI/IComputeState.h>

namespace Methane::Graphics::META_GFX_NAME
{
class ComputeState;
}

namespace Methane::Graphics::Rhi
{

struct ComputeStateSettingsImpl
{
    Program            program;

    META_PIMPL_API static ComputeStateSettings Convert(const ComputeStateSettingsImpl& settings);
};

class RenderContext;
class ComputeContext;

class ComputeState // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Settings   = ComputeStateSettingsImpl;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(ComputeState);
    META_PIMPL_METHODS_COMPARE_DECLARE(ComputeState);

    META_PIMPL_API explicit ComputeState(const Ptr<IComputeState>& interface_ptr);
    META_PIMPL_API explicit ComputeState(IComputeState& interface_ref);
    META_PIMPL_API ComputeState(const RenderContext& context, const Settings& settings);
    META_PIMPL_API ComputeState(const ComputeContext& context, const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IComputeState& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IComputeState> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IComputeState interface methods
    [[nodiscard]] META_PIMPL_API const ComputeStateSettings& GetSettings() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void Reset(const Settings& settings) const;
    META_PIMPL_API void Reset(const IComputeState::Settings& settings) const;

    META_PIMPL_API Program GetProgram() const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::ComputeState;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/ComputeState.cpp>

#endif // META_PIMPL_INLINE
