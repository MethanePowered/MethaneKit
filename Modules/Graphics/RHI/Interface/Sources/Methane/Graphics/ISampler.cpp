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

FILE: Methane/Graphics/ISampler.cpp
Methane sampler interface: GPU resource for texture sampling.

******************************************************************************/

#include <Methane/Graphics/ISampler.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

SamplerSettings::SamplerSettings(const SamplerFilter& filter, const SamplerAddress& address,
                                 const SamplerLevelOfDetail& lod, uint32_t max_anisotropy,
                                 SamplerBorderColor border_color, Compare compare_function)
    : filter(filter)
    , address(address)
    , lod(lod)
    , max_anisotropy(max_anisotropy)
    , border_color(border_color)
    , compare_function(compare_function)
{
    META_FUNCTION_TASK();
}

SamplerLevelOfDetail::SamplerLevelOfDetail(float bias, float min, float max)
    : min(min)
    , max(max)
    , bias(bias)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
