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

FILE: Methane/Graphics/RHI/IDescriptorManager.h
Descriptor manager interface.

******************************************************************************/

#pragma once

#include <cstdint>

namespace Methane::Data
{

using Index = uint32_t;

} // namespace Methane::Data

// FIXME: DirectX specific types should not be declared in the Base layer
namespace Methane::Graphics::DirectX
{

class DescriptorHeap;

struct ResourceDescriptor
{
    DescriptorHeap& heap;
    Data::Index     index;

    ResourceDescriptor(DescriptorHeap& in_heap, Data::Index in_index);
};

} // namespace Methane::Graphics::DirectX

namespace Methane::Graphics::Rhi
{

struct IProgramBindings;

struct IDescriptorManager
{
    virtual void AddProgramBindings(IProgramBindings& program_bindings) = 0;
    virtual void RemoveProgramBindings(IProgramBindings& program_bindings) = 0;
    virtual void CompleteInitialization() = 0;
    virtual void Release() = 0;

    virtual ~IDescriptorManager() = default;
};

} // namespace Methane::Graphics::Rhi
