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

FILE: Methane/Graphics/Buffer.h
Methane buffer interface: GPU memory buffer resource.

******************************************************************************/

#pragma once

#include "Resource.h"

namespace Methane::Graphics
{

struct Context;

struct Buffer : virtual Resource // NOSONAR
{
    enum class Type
    {
        Data = 0,
        Index,
        Vertex,
        Constant,
        ReadBack,
    };

    enum class StorageMode
    {
        Managed, // CPU-GPU buffer with automatic data synchronization managed by graphics runtime
        Private, // Private GPU buffer asynchronously uploaded through the intermediate shared CPU-GPU buffer
    };

    struct Settings
    {
        Buffer::Type type;
        Usage        usage_mask;
        Data::Size   size;
        Data::Size   item_stride_size;
        PixelFormat  data_format;
        StorageMode  storage_mode = StorageMode::Managed;
    };

    // Create Buffer instance
    [[nodiscard]] static Ptr<Buffer> CreateVertexBuffer(const Context& context, Data::Size size, Data::Size stride, bool is_volatile = false);
    [[nodiscard]] static Ptr<Buffer> CreateIndexBuffer(const Context& context, Data::Size size, PixelFormat format, bool is_volatile = false);
    [[nodiscard]] static Ptr<Buffer> CreateConstantBuffer(const Context& context, Data::Size size, bool addressable = false, bool is_volatile = false, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());
    [[nodiscard]] static Ptr<Buffer> CreateReadBackBuffer(const Context& context, Data::Size size);

    // Auxiliary functions
    [[nodiscard]] static Data::Size  GetAlignedBufferSize(Data::Size size) noexcept;

    // Buffer interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    [[nodiscard]] virtual uint32_t        GetFormattedItemsCount() const noexcept = 0;
};

struct BufferSet
{
    [[nodiscard]] static Ptr<BufferSet> Create(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs);
    [[nodiscard]] static Ptr<BufferSet> CreateVertexBuffers(const Refs<Buffer>& buffer_refs) { return BufferSet::Create(Buffer::Type::Vertex, buffer_refs); }

    [[nodiscard]] virtual Buffer::Type        GetType() const noexcept = 0;
    [[nodiscard]] virtual Data::Size          GetCount() const noexcept = 0;
    [[nodiscard]] virtual const Refs<Buffer>& GetRefs() const noexcept = 0;
    [[nodiscard]] virtual std::string         GetNames() const noexcept = 0;
    [[nodiscard]] virtual Buffer&             operator[](Data::Index index) const = 0;

    virtual ~BufferSet() = default;
};

} // namespace Methane::Graphics
