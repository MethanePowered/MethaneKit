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

FILE: Methane/Graphics/Metal/ContextMT.h
Vulkan context accessor interface for template class ContextMT<ContextBaseT>

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

class DeviceVK;
class CommandQueueVK;

struct IContextVK
{
    virtual DeviceVK&       GetDeviceVK() noexcept = 0;
    virtual CommandQueueVK& GetUploadCommandQueueVK() noexcept = 0;
};

} // namespace Methane::Graphics
