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

FILE: Methane/Graphics/SamplerBase.cpp
Base implementation of the sampler interface.

******************************************************************************/

#include "SamplerBase.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Sampler::Settings::Settings(const Filter& in_filter, const Address& in_address,
                            const LevelOfDetail& in_lod, uint32_t in_max_anisotropy,
                            BorderColor in_border_color, Compare in_compare_function)
    : filter(in_filter)
    , address(in_address)
    , lod(in_lod)
    , max_anisotropy(in_max_anisotropy)
    , border_color(in_border_color)
    , compare_function(in_compare_function)
{
}

Sampler::LevelOfDetail::LevelOfDetail(float in_bias, float in_min, float in_max)
    : min(in_min)
    , max(in_max)
    , bias(in_bias)
{
}

SamplerBase::SamplerBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceNT(Type::Sampler, Usage::ShaderRead, context, descriptor_by_usage)
    , m_context(context)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

void SamplerBase::SetData(const SubResources&)
{
    ITT_FUNCTION_TASK();
    throw std::logic_error("Samplers do not support setting the data.");
}

} // namespace Methane::Graphics