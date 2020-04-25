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

FILE: Methane/Graphics/Buffer.h
Methane buffer interface: GPU memory buffer resource.

******************************************************************************/

#pragma once

#include "Resource.h"

namespace Methane::Graphics
{

struct Context;

struct Buffer : virtual Resource
{
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
        Data::Size   item_stride_size;
        PixelFormat  data_format;
    };

    // Create Buffer instance
    static Ptr<Buffer> CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride);
    static Ptr<Buffer> CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format);
    static Ptr<Buffer> CreateConstantBuffer(Context& context, Data::Size size, bool addressable = false, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Auxiliary functions
    static Data::Size  GetAlignedBufferSize(Data::Size size) noexcept;
    static std::string GetBufferTypeName(Type type) noexcept;

    // Buffer interface
    virtual const Settings& GetSettings() const noexcept = 0;
    virtual uint32_t        GetFormattedItemsCount() const noexcept = 0;
};

struct Buffers
{
    static Ptr<Buffers> Create(Buffer::Type buffers_type, Refs<Buffer> buffer_refs);
    static Ptr<Buffers> CreateVertexBuffers(Refs<Buffer> buffer_refs) { return Buffers::Create(Buffer::Type::Vertex, std::move(buffer_refs)); }

    virtual Buffer::Type        GetType() const noexcept = 0;
    virtual Data::Size          GetCount() const noexcept = 0;
    virtual const Refs<Buffer>& GetRefs() const noexcept = 0;
    virtual Buffer&             operator[](Data::Index index) const = 0;

    virtual ~Buffers() = default;
};

} // namespace Methane::Graphics
