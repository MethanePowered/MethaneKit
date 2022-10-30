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

FILE: Methane/Graphics/TransferCommandList.h
Methane data transfer command list interface.

******************************************************************************/

#pragma once

#include "CommandList.h"

#include <Methane/Memory.hpp>

namespace Methane::Graphics
{

struct TransferCommandList : virtual CommandList // NOSONAR
{
    static constexpr Type type = Type::Transfer;

    // Create TransferCommandList instance
    [[nodiscard]] static Ptr<TransferCommandList> Create(ICommandQueue& command_queue);

    // No public functions here for now, transfer command lists are used internally only
    // Later it will include memory copy operations and mip-map generation for textures on GPU
};

} // namespace Methane::Graphics
