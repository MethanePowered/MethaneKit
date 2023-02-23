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

FILE: Methane/Graphics/RHI/IComputeContext.h
Methane render context interface: represents graphics device and swap chain,
provides basic multi-frame rendering synchronization and frame presenting APIs.

******************************************************************************/

#pragma once

#include "IContext.h"

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Platform/AppEnvironment.h>
#include <Methane/Platform/AppView.h>
#include <Methane/Memory.hpp>

#include <optional>

namespace Methane::Graphics::Rhi
{

struct ComputeContextSettings
{
};

struct IComputeContext
    : virtual IContext // NOSONAR
{
    using Settings = ComputeContextSettings;

    // Create IComputeContext instance
    [[nodiscard]] static Ptr<IComputeContext> Create(IDevice& device, tf::Executor& parallel_executor, const Settings& settings);

    // IComputeContext interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;

    [[nodiscard]] ICommandKit& GetComputeCommandKit() const;
};

} // namespace Methane::Graphics::Rhi
