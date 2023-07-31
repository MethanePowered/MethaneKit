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

FILE: Methane/Graphics/RHI/IBuffer.h
Methane buffer interface: GPU memory buffer resource.

******************************************************************************/

#pragma once

#include "IResource.h"

namespace Methane::Graphics::Rhi
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
    ResourceUsageMask usage_mask;
    Data::Size        size;
    Data::Size        item_stride_size;
    PixelFormat       data_format;
    BufferStorageMode storage_mode = BufferStorageMode::Managed;

    [[nodiscard]] static BufferSettings ForVertexBuffer(Data::Size size, Data::Size stride, bool is_volatile = false);
    [[nodiscard]] static BufferSettings ForIndexBuffer(Data::Size size, PixelFormat format, bool is_volatile = false);
    [[nodiscard]] static BufferSettings ForConstantBuffer(Data::Size size, bool addressable = false, bool is_volatile = false);
    [[nodiscard]] static BufferSettings ForReadBackBuffer(Data::Size size);

    bool operator==(const BufferSettings& other) const;
    bool operator!=(const BufferSettings& other) const;
};

struct IBuffer
    : virtual IResource // NOSONAR
{
    using Type        = BufferType;
    using StorageMode = BufferStorageMode;
    using Settings    = BufferSettings;

    // Create IBuffer instance
    [[nodiscard]] static Ptr<IBuffer> Create(const IContext& context, const Settings& settings);

    // IBuffer interface
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    [[nodiscard]] virtual uint32_t        GetFormattedItemsCount() const noexcept = 0;
    [[nodiscard]] virtual SubResource     GetData(ICommandQueue& target_cmd_queue, const BytesRangeOpt& data_range = {}) = 0;
    virtual void SetData(ICommandQueue& target_cmd_queue, const SubResource& sub_resource) = 0;
};

} // namespace Methane::Graphics::Rhi
