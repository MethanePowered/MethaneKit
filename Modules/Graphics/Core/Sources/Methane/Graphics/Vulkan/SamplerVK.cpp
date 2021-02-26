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

FILE: Methane/Graphics/Vulkan/SamplerVK.mm
Vulkan implementation of the sampler interface.

******************************************************************************/

#include "SamplerVK.h"
#include "ContextVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<Sampler> Sampler::Create(Context& context, const Sampler::Settings& settings, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    return std::make_shared<SamplerVK>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

SamplerVK::SamplerVK(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceVK(context, settings, descriptor_by_usage)
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
    
    ResetSamplerState();
}

void SamplerVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ResourceVK::SetName(name);

    ResetSamplerState();
}

void SamplerVK::ResetSamplerState()
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
