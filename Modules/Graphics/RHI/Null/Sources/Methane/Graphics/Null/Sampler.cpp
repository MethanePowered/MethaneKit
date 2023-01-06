/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/Sampler.cpp
Null implementation of the sampler interface.

******************************************************************************/

#include <Methane/Graphics/Null/Sampler.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<ISampler> ISampler::Create(const IContext& context, const ISampler::Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Null::Sampler>(dynamic_cast<const Base::Context&>(context), settings);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Null
{

Sampler::Sampler(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics::Null
