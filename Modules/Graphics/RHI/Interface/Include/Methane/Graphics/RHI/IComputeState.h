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

FILE: Methane/Graphics/RHI/IComputeState.h
Methane compute state interface: specifies configuration of the compute pipeline.

******************************************************************************/

#pragma once

#include "IObject.h"

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Volume.hpp>
#include <Methane/Memory.hpp>

namespace Methane::Graphics::Rhi
{

struct IProgram;

using ThreadGroupSize = VolumeSize<uint32_t>;

struct ComputeStateSettings
{
    Ptr<IProgram>   program_ptr;
    ThreadGroupSize thread_group_size; // Duplicated in HLSL attribute [numthreads(x,y,z)] of compute shader, but Metal does not use it

    [[nodiscard]] bool operator==(const ComputeStateSettings& other) const noexcept;
    [[nodiscard]] bool operator!=(const ComputeStateSettings& other) const noexcept;
    [[nodiscard]] explicit operator std::string() const;
};

struct IContext;

struct IComputeState
    : virtual IObject // NOSONAR
{
public:
    using Settings = ComputeStateSettings;

    // Create IComputeState instance
    [[nodiscard]] static Ptr<IComputeState> Create(const IContext& device, const Settings& state_settings);

    // IComputeState interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    virtual void Reset(const Settings& settings) = 0;
};

} // namespace Methane::Graphics::Rhi
