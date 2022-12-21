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

FILE: Methane/Graphics/RHI/Sampler.h
Methane Sampler PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include "Pimpl.h"

#include <Methane/Graphics/RHI/ISampler.h>
#include <Methane/Data/Transmitter.hpp>

namespace Methane::Graphics::Rhi
{

class RenderContext;

class Sampler
    : public Data::Transmitter<Rhi::IObjectCallback>
    , public Data::Transmitter<Rhi::IResourceCallback>
{
public:
    using Filter        = SamplerFilter;
    using Address       = SamplerAddress;
    using LevelOfDetail = SamplerLevelOfDetail;
    using BorderColor   = SamplerBorderColor;
    using Settings      = SamplerSettings;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Sampler);

    Sampler(const Ptr<ISampler>& interface_ptr);
    Sampler(ISampler& interface_ref);
    Sampler(const RenderContext& context, const Settings& settings);

    void Init(const RenderContext& context, const Settings& settings);
    void Release();

    bool IsInitialized() const META_PIMPL_NOEXCEPT;
    ISampler& GetInterface() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    bool SetName(std::string_view name) const;
    std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // ISampler interface methods
    [[nodiscard]] const Settings& GetSettings() const META_PIMPL_NOEXCEPT;

private:
    class Impl;

    Sampler(UniquePtr<Impl>&& impl_ptr);

    UniquePtr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi
