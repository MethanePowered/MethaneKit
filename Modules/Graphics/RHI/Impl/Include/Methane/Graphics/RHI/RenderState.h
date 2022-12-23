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

#include "Pimpl.h"
#include "Program.h"
#include "RenderPass.h"

#include <Methane/Graphics/RHI/IRenderState.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class RenderContext;

class RenderState
    : public Data::Transmitter<Rhi::IObjectCallback>
{
public:
    using Rasterizer = RasterizerSettings;
    using Blending   = BlendingSettings;
    using Depth      = DepthSettings;
    using Stencil    = StencilSettings;
    using Groups     = RenderStateGroupMask;
    using Group      = RenderStateGroup;

    struct Settings
    {
        const Program&       program;
        const RenderPattern& render_pattern;
        RasterizerSettings   rasterizer;
        DepthSettings        depth;
        StencilSettings      stencil;
        BlendingSettings     blending;
        Color4F              blending_color;
    };

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderState);

    RenderState(const Ptr<IRenderState>& interface_ptr);
    RenderState(IRenderState& interface_ref);
    RenderState(const RenderContext& context, const Settings& settings);

    void Init(const RenderContext& context, const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    IRenderState& GetInterface() const META_PIMPL_NOEXCEPT;
    Ptr<IRenderState> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // IRenderState interface methods
    [[nodiscard]] const RenderStateSettings& GetSettings() const META_PIMPL_NOEXCEPT;
    void Reset(const Settings& settings) const;

private:
    class Impl;

    RenderState(UniquePtr<Impl>&& impl_ptr);

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
