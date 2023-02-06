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

FILE: Methane/Graphics/Null/Sampler.h
Null implementation of the sampler interface.

******************************************************************************/

#pragma once

#include "Resource.hpp"

#include <Methane/Graphics/Base/Sampler.h>

namespace Methane::Graphics::Null
{

struct IContext;

class Sampler final // NOSONAR - inheritance hierarchy is greater than 5
    : public Resource<Base::Sampler>
{
public:
    Sampler(const Base::Context& context, const Settings& settings);
};

} // namespace Methane::Graphics::Null
