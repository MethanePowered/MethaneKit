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

FILE: Methane/Graphics/Base/Sampler.cpp
Base implementation of the sampler interface.

******************************************************************************/

#include <Methane/Graphics/Base/Sampler.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Base
{

Sampler::Sampler(const Context& context, const Settings& settings,
                 State initial_state, Opt<State> auto_transition_source_state_opt)
    : Resource(context, Type::Sampler, UsageMask(Usage::ShaderRead), initial_state, auto_transition_source_state_opt)
    , m_settings(settings)
{ }

} // namespace Methane::Graphics::Base