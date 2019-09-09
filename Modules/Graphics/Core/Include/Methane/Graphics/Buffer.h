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

FILE: Methane/Graphics/Buffer.h
Methane buffer interface: GPU memory buffer resource.

******************************************************************************/

#pragma once

#include "Resource.h"

#include <vector>

namespace Methane::Graphics
{

struct Buffer : virtual Resource
{
    using Ptr     = std::shared_ptr<Buffer>;
    using WeakPtr = std::weak_ptr<Buffer>;
    using Ref     = std::reference_wrapper<Buffer>;
    using Refs    = std::vector<Ref>;

    enum class Type
    {
        Data = 0,
        Index,
        Vertex,
        Constant,
    };

    struct Settings
    {
        Buffer::Type type;
        Usage::Mask  usage_mask;
        Data::Size   size;
    };

    // Create Buffer instance
    static Ptr CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride);
    static Ptr CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format);
    static Ptr CreateConstantBuffer(Context& context, Data::Size size, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Auxillary functions
    static Data::Size  GetAlignedBufferSize(Data::Size size) noexcept;
    static std::string GetBufferTypeName(Type type) noexcept;

    // Buffer interface
    virtual uint32_t     GetFormattedItemsCount() const = 0;
    virtual Buffer::Type GetBufferType() const noexcept = 0;
};

} // namespace Methane::Graphics
