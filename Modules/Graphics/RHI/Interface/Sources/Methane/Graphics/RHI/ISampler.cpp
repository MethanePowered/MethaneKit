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

namespace Methane::Graphics::Rhi
{

SamplerLevelOfDetail::SamplerLevelOfDetail(float bias, float min, float max)
    : min(min)
    , max(max)
    , bias(bias)
{ }

Ptr<ISampler> ISampler::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return context.CreateSampler(settings);
}

} // namespace Methane::Graphics::Rhi
