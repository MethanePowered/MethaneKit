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

FILE: Methane/Graphics/RHI/ISampler.cpp
Methane sampler interface: GPU resource for texture sampling.

******************************************************************************/

#include <Methane/Graphics/RHI/ISampler.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

bool SamplerFilter::operator==(const SamplerFilter& other) const
{
    return std::tie(min, mag, mip)
        == std::tie(other.min, other.mag, other.mip);
}

bool SamplerFilter::operator!=(const SamplerFilter& other) const
{
    return std::tie(min, mag, mip)
        != std::tie(other.min, other.mag, other.mip);
}

bool SamplerAddress::operator==(const SamplerAddress& other) const
{
    return std::tie(s, t, r)
        == std::tie(other.s, other.t, other.r);
}

bool SamplerAddress::operator!=(const SamplerAddress& other) const
{
    return std::tie(s, t, r)
        != std::tie(other.s, other.t, other.r);
}

SamplerSettings::SamplerSettings(const SamplerFilter& filter, const SamplerAddress& address,
                                 const SamplerLevelOfDetail& lod, uint32_t max_anisotropy,
                                 SamplerBorderColor border_color, Compare compare_function)
    : filter(filter)
    , address(address)
    , lod(lod)
    , max_anisotropy(max_anisotropy)
    , border_color(border_color)
    , compare_function(compare_function)
{ }

bool SamplerSettings::operator==(const SamplerSettings& other) const
{
    return std::tie(filter, address, lod, max_anisotropy, border_color, compare_function)
        == std::tie(other.filter, other.address, other.lod, other.max_anisotropy, other.border_color, other.compare_function);
}

bool SamplerSettings::operator!=(const SamplerSettings& other) const
{
    return std::tie(filter, address, lod, max_anisotropy, border_color, compare_function)
        != std::tie(other.filter, other.address, other.lod, other.max_anisotropy, other.border_color, other.compare_function);
}

SamplerLevelOfDetail::SamplerLevelOfDetail(float bias, float min, float max)
    : min(min)
    , max(max)
    , bias(bias)
{ }

bool SamplerLevelOfDetail::operator==(const SamplerLevelOfDetail& other) const
{
    return std::tie(min, max, bias)
        == std::tie(other.min, other.max, other.bias);
}

bool SamplerLevelOfDetail::operator!=(const SamplerLevelOfDetail& other) const
{
    return std::tie(min, max, bias)
        != std::tie(other.min, other.max, other.bias);
}

Ptr<ISampler> ISampler::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return context.CreateSampler(settings);
}

} // namespace Methane::Graphics::Rhi
