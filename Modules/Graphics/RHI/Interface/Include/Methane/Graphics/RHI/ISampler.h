/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/ISampler.h
Methane sampler interface: GPU resource for texture sampling.

******************************************************************************/

#pragma once

#include "IResource.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>
#include <Methane/Instrumentation.h>

#include <string>
#include <limits>

namespace Methane::Graphics::Rhi
{

struct SamplerFilter
{
    enum class MinMag : uint32_t
    {
        Nearest = 0,
        Linear
    };

    enum class Mip : uint32_t
    {
        NotMipmapped = 0,
        Nearest,
        Linear
    };

    SamplerFilter(MinMag min, MinMag mag, Mip mip) : min(min), mag(mag), mip(mip) { }
    SamplerFilter(MinMag min_mag, Mip mip) : SamplerFilter(min_mag, min_mag, mip) { }
    explicit SamplerFilter(MinMag min_mag) : SamplerFilter(min_mag, Mip::NotMipmapped) { }

    [[nodiscard]] friend bool operator==(const SamplerFilter& left, const SamplerFilter& right)
    {
        META_FUNCTION_TASK();
        return std::tie(left.min, left.mag, left.mip) ==
               std::tie(right.min, right.mag, right.mip);
    }

    [[nodiscard]] friend bool operator!=(const SamplerFilter& left, const SamplerFilter& right)
    {
        return !(left == right);
    }


    MinMag min = MinMag::Nearest;
    MinMag mag = MinMag::Nearest;
    Mip    mip = Mip::NotMipmapped;
};

struct SamplerAddress
{
    enum class Mode : uint32_t
    {
        ClampToEdge = 0,
        ClampToZero,
        ClampToBorderColor,
        Repeat,
        RepeatMirror,
    };

    SamplerAddress(Mode s, Mode t, Mode r) : s(s), t(t), r(r) { }
    explicit SamplerAddress(Mode all) : s(all), t(all), r(all) { }

    [[nodiscard]] friend bool operator==(const SamplerAddress& left, const SamplerAddress& right)
    {
        META_FUNCTION_TASK();
        return std::tie(left.s, left.t, left.r) ==
               std::tie(right.s, right.t, right.r);
    }

    [[nodiscard]] friend bool operator!=(const SamplerAddress& left, const SamplerAddress& right)
    {
        return !(left == right);
    }

    Mode s = Mode::ClampToEdge; // width
    Mode t = Mode::ClampToEdge; // height
    Mode r = Mode::ClampToEdge; // depth
};

struct SamplerLevelOfDetail
{
    SamplerLevelOfDetail(float bias = 0.F, float min = 0.F, float max = std::numeric_limits<float>::max());

    [[nodiscard]] friend bool operator==(const SamplerLevelOfDetail& left, const SamplerLevelOfDetail& right)
    {
        META_FUNCTION_TASK();
        return std::tie(left.min, left.max, left.bias) ==
               std::tie(right.min, right.max, right.bias);
    }

    [[nodiscard]] friend bool operator!=(const SamplerLevelOfDetail& left, const SamplerLevelOfDetail& right)
    {
        return !(left == right);
    }

    float min  = 0.F;
    float max  = std::numeric_limits<float>::max();
    float bias = 0.F;
};

enum class SamplerBorderColor : uint32_t
{
    TransparentBlack = 0,
    OpaqueBlack,
    OpaqueWhite,
};

struct SamplerSettings
{
    SamplerSettings(const SamplerFilter& filter,
                    const SamplerAddress& address,
                    const SamplerLevelOfDetail& lod = {},
                    uint32_t  max_anisotropy = 1,
                    SamplerBorderColor border_color = SamplerBorderColor::TransparentBlack,
                    Compare compare_function = Compare::Never);

    [[nodiscard]] friend bool operator==(const SamplerSettings& left, const SamplerSettings& right)
    {
        return std::tie(left.filter, left.address, left.lod, left.max_anisotropy, left.border_color, left.compare_function) ==
               std::tie(right.filter, right.address, right.lod, right.max_anisotropy, right.border_color, right.compare_function);
    }

    [[nodiscard]] friend bool operator!=(const SamplerSettings& left, const SamplerSettings& right)
    {
        return !(left == right);
    }

    SamplerFilter        filter;
    SamplerAddress       address;
    SamplerLevelOfDetail lod;
    uint32_t             max_anisotropy   = 1;
    SamplerBorderColor   border_color     = SamplerBorderColor::TransparentBlack;
    Compare              compare_function = Compare::Never;
};

struct IContext;

struct ISampler
    : virtual IResource // NOSONAR
{
    using Filter        = SamplerFilter;
    using Address       = SamplerAddress;
    using LevelOfDetail = SamplerLevelOfDetail;
    using BorderColor   = SamplerBorderColor;
    using Settings      = SamplerSettings;

    // Create ISampler instance
    [[nodiscard]] static Ptr<ISampler> Create(const IContext& context, const Settings& settings);

    // ISampler interface
    [[nodiscard]] virtual const Settings& GetSettings() const = 0;
};

} // namespace Methane::Graphics::Rhi
