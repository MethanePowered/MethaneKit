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

FILE: Methane/Graphics/Vulkan/SamplerVK.cpp
Vulkan implementation of the sampler interface.

******************************************************************************/

#include "SamplerVK.h"
#include "ContextVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<Sampler> Sampler::Create(const Context& context, const Sampler::Settings& settings, const DescriptorByUsage&)
{
    META_FUNCTION_TASK();
    return std::make_shared<SamplerVK>(dynamic_cast<const ContextBase&>(context), settings);
}

SamplerVK::SamplerVK(const ContextBase& context, const Settings& settings)
    : ResourceVK(context, settings, {})
{
    META_FUNCTION_TASK();
    ResetSamplerState();
}

void SamplerVK::ResetSamplerState()
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
