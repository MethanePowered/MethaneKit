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

FILE: Methane/Graphics/Vulkan/ComputeContext.hh
Vulkan implementation of the compute context interface.

******************************************************************************/

#pragma once

#include "Context.hpp"

#include <Methane/Graphics/Base/ComputeContext.h>

namespace Methane::Graphics::Vulkan
{

class ComputeContext final
    : public Context<Base::ComputeContext>
{
public:
    using Context::Context;
};

} // namespace Methane::Graphics::Vulkan
