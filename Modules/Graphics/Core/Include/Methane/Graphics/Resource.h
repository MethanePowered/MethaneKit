/******************************************************************************

Copyright 2019 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Resource.h
Methane resource interface: base class of all GPU resources.

******************************************************************************/

#pragma once

#include "Object.h"
#include "Types.h"

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <array>

namespace Methane
{
namespace Graphics
{

struct Context;
class DescriptorHeap;

struct Resource : virtual Object
{
    using Ptr     = std::shared_ptr<Resource>;
    using WeakPtr = std::weak_ptr<Resource>;
    using Ref     = std::reference_wrapper<Resource>;
    using Refs    = std::vector<Ref>;

    enum class Type
    {
        Buffer,
        Texture,
        Sampler,
    };

    struct Usage
    {
        using Mask = uint32_t;
        enum Value : Mask
        {
            Unknown      = 0,
            ShaderRead   = 1 << 0,
            ShaderWrite  = 1 << 1,
            RenderTarget = 1 << 2,
            All          = static_cast<Mask>(~0),
        };

        using Values = std::array<Value, 3>;
        static constexpr const Values values = { ShaderRead, ShaderWrite, RenderTarget };

        Usage() = delete;
        ~Usage() = delete;
    };

    struct Descriptor
    {
        DescriptorHeap& heap;
        int32_t         index;

        Descriptor(DescriptorHeap& in_heap, int32_t in_index = -1)
            : heap(in_heap)
            , index(in_index)
        { }
    };

    using DescriptorByUsage = std::map<Usage::Value, Descriptor>;

    // Auxillary functions
    static std::string GetTypeName(Type type) noexcept;
    static std::string GetUsageName(Usage::Value usage) noexcept;
    static std::string GetUsageNames(Usage::Mask usage_mask) noexcept;

    // Resource interface
    virtual void                      SetData(Data::ConstRawPtr p_data, Data::Size data_size) = 0;
    virtual Data::Size                GetDataSize() const = 0;
    virtual Type                      GetResourceType() const noexcept = 0;
    virtual Usage::Mask               GetUsageMask() const noexcept = 0;
    virtual const DescriptorByUsage&  GetDescriptorByUsage() const noexcept = 0;
    virtual const Descriptor&         GetDescriptor(Usage::Value usage) const = 0;

    virtual ~Resource() override = default;
};

} // namespace Graphics
} // namespace Methane
