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

FILE: Methane/Graphics/RHI/RenderState.h
Methane RenderState PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>
#include "Program.h"
#include "RenderPass.h"
#include "RenderPattern.h"

#include <Methane/Graphics/RHI/IRenderState.h>

namespace Methane::Graphics::META_GFX_NAME
{
class RenderState;
}

namespace Methane::Graphics::Rhi
{

struct RenderStateSettingsImpl
{
    Program            program;
    RenderPattern      render_pattern;
    RasterizerSettings rasterizer;
    DepthSettings      depth;
    StencilSettings    stencil;
    BlendingSettings   blending;
    Color4F            blending_color;

    META_PIMPL_API static RenderStateSettings Convert(const RenderStateSettingsImpl& settings);
};

class RenderContext;

class RenderState // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Settings   = RenderStateSettingsImpl;
    using Rasterizer = RasterizerSettings;
    using Blending   = BlendingSettings;
    using Depth      = DepthSettings;
    using Stencil    = StencilSettings;
    using Groups     = RenderStateGroupMask;
    using Group      = RenderStateGroup;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderState);
    META_PIMPL_METHODS_COMPARE_DECLARE(RenderState);

    META_PIMPL_API explicit RenderState(const Ptr<IRenderState>& interface_ptr);
    META_PIMPL_API explicit RenderState(IRenderState& interface_ref);
    META_PIMPL_API RenderState(const RenderContext& context, const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IRenderState& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IRenderState> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IRenderState interface methods
    [[nodiscard]] META_PIMPL_API const RenderStateSettings& GetSettings() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void Reset(const Settings& settings) const;
    META_PIMPL_API void Reset(const IRenderState::Settings& settings) const;

    META_PIMPL_API Program       GetProgram() const;
    META_PIMPL_API RenderPattern GetRenderPattern() const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::RenderState;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/RenderState.cpp>

#endif // META_PIMPL_INLINE
