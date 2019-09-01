/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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
#include "Instrumentation.h"

namespace Methane
{
namespace Graphics
{

SamplerBase::SamplerBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceNT(Type::Sampler, Usage::ShaderRead, context, descriptor_by_usage)
    , m_context(context)
    , m_settings(settings)
{
    ITT_FUNCTION_TASK();
}

} // namespace Graphics
} // namespace Methane