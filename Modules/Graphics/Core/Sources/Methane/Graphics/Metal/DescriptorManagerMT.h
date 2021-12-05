/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/DescriptorManagerMT.h
Dummy Metal resource manager.

******************************************************************************/

#pragma once

#include <Methane/Graphics/DescriptorManager.h>

namespace Methane::Graphics
{

struct DescriptorManagerMT : DescriptorManager
{
    void CompleteInitialization() final {}
    void Release() final {}
};

} // namespace Methane::Graphics
