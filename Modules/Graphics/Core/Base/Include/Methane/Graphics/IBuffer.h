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

#include "IResource.h"

namespace Methane::Graphics
{

struct IContext;

enum class BufferType
{
    Constant = 0,
    Storage,
    Index,
    Vertex,
    ReadBack
};

enum class BufferStorageMode
{
    Managed, // CPU-GPU buffer with automatic data synchronization managed by graphics runtime
    Private, // Private GPU buffer asynchronously uploaded through the intermediate shared CPU-GPU buffer
};

struct BufferSettings
{
    BufferType        type;
    ResourceUsage     usage_mask;
    Data::Size        size;
    Data::Size        item_stride_size;
    PixelFormat       data_format;
    BufferStorageMode storage_mode = BufferStorageMode::Managed;
};

struct IBuffer
    : virtual IResource // NOSONAR
{
    using Type = BufferType;
    using StorageMode = BufferStorageMode;
    using Settings = BufferSettings;

    // Create IBuffer instance
    [[nodiscard]] static Ptr<IBuffer> CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile = false);
    [[nodiscard]] static Ptr<IBuffer> CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile = false);
    [[nodiscard]] static Ptr<IBuffer> CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable = false, bool is_volatile = false);
    [[nodiscard]] static Ptr<IBuffer> CreateReadBackBuffer(const IContext& context, Data::Size size);

    // Auxiliary functions
    [[nodiscard]] static Data::Size  GetAlignedBufferSize(Data::Size size) noexcept;

    // IBuffer interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    [[nodiscard]] virtual uint32_t        GetFormattedItemsCount() const noexcept = 0;
};

struct IBufferSet
{
    [[nodiscard]] static Ptr<IBufferSet> Create(IBuffer::Type buffers_type, const Refs<IBuffer>& buffer_refs);
    [[nodiscard]] static Ptr<IBufferSet> CreateVertexBuffers(const Refs<IBuffer>& buffer_refs) { return IBufferSet::Create(IBuffer::Type::Vertex, buffer_refs); }

    [[nodiscard]] virtual IBuffer::Type        GetType() const noexcept = 0;
    [[nodiscard]] virtual Data::Size           GetCount() const noexcept = 0;
    [[nodiscard]] virtual const Refs<IBuffer>& GetRefs() const noexcept = 0;
    [[nodiscard]] virtual std::string          GetNames() const noexcept = 0;
    [[nodiscard]] virtual IBuffer&             operator[](Data::Index index) const = 0;

    virtual ~IBufferSet() = default;
};

} // namespace Methane::Graphics
