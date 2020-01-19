/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Sampler.h
Methane sampler interface: GPU resource for texture sampling.

******************************************************************************/

#pragma once

#include "Resource.h"
#include "Types.h"

#include <Methane/Memory.hpp>

#include <string>
#include <limits>

namespace Methane::Graphics
{

struct Context;

struct Sampler : virtual Resource
{
    struct Filter
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
        
        Filter(MinMag in_min, MinMag in_mag, Mip in_mip) : min(in_min), mag(in_mag), mip(in_mip) { }
        Filter(MinMag in_min_max, Mip in_mip) : Filter(in_min_max, in_min_max, in_mip) { }
        Filter(MinMag in_min_max) : Filter(in_min_max, Mip::NotMipmapped) { }

        MinMag min = MinMag::Nearest;
        MinMag mag = MinMag::Nearest;
        Mip    mip = Mip::NotMipmapped;
    };

    struct Address
    {
        enum class Mode : uint32_t
        {
            ClampToEdge = 0,
            ClampToZero,
            ClampToBorderColor,
            Repeat,
            RepeatMirror,
        };
        
        Address(Mode in_s, Mode in_t, Mode in_r) : s(in_s), t(in_t), r(in_r) { }
        Address(Mode all) : s(all), t(all), r(all) { }

        Mode s = Mode::ClampToEdge; // width
        Mode t = Mode::ClampToEdge; // height
        Mode r = Mode::ClampToEdge; // depth
    };

    struct LevelOfDetail
    {
        LevelOfDetail(float in_bias = 0.f, float in_min = 0.f, float in_max = std::numeric_limits<float>::max());
        
        float min     = 0.f;
        float max     = std::numeric_limits<float>::max();
        float bias    = 0.f;
    };

    enum class BorderColor : uint32_t
    {
        TransparentBlack = 0,
        OpaqueBlack,
        OpaqueWhite,
    };

    struct Settings
    {
        Settings(const Filter& in_filter,
                 const Address& in_address,
                 const LevelOfDetail& in_lod = LevelOfDetail(),
                 uint32_t  in_max_anisotropy = 1,
                 BorderColor in_border_color = BorderColor::TransparentBlack,
                 Compare in_compare_function = Compare::Never);

        Filter          filter;
        Address         address;
        LevelOfDetail   lod;
        uint32_t        max_anisotropy      = 1;
        BorderColor     border_color        = BorderColor::TransparentBlack;
        Compare         compare_function    = Compare::Never;
    };

    // Create Sampler instance
    static Ptr<Sampler> Create(Context& context, const Settings& state_settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Sampler interface
    virtual const Settings& GetSettings() const = 0;
};

} // namespace Methane::Graphics
