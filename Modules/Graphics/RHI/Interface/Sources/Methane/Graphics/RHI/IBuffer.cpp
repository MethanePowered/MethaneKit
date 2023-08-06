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

FILE: Methane/Graphics/RHI/IBuffer.cpp
Methane buffer interface: GPU memory buffer resource.

******************************************************************************/

#include <Methane/Graphics/RHI/IBuffer.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<IBuffer> IBuffer::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return context.CreateBuffer(settings);
}

inline Rhi::BufferStorageMode GetBufferStorageMode(bool is_volatile_data) noexcept
{
    return is_volatile_data ? Rhi::BufferStorageMode::Managed
                            : Rhi::BufferStorageMode::Private;
}

BufferSettings BufferSettings::ForVertexBuffer(Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Rhi::BufferSettings{
        Rhi::BufferType::Vertex,
        Rhi::ResourceUsageMask(),
        size,
        stride,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
}

BufferSettings BufferSettings::ForIndexBuffer(Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Rhi::BufferSettings{
        Rhi::BufferType::Index,
        Rhi::ResourceUsageMask(),
        size,
        GetPixelSize(format),
        format,
        GetBufferStorageMode(is_volatile)
    };
}

BufferSettings BufferSettings::ForConstantBuffer(Data::Size size, bool addressable, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Rhi::BufferSettings{
        Rhi::BufferType::Constant,
        Rhi::ResourceUsageMask(Rhi::ResourceUsage::ShaderRead).SetBit(Rhi::ResourceUsage::Addressable, addressable),
        size,
        0U,
        PixelFormat::Unknown,
        GetBufferStorageMode(is_volatile)
    };
}

BufferSettings BufferSettings::ForReadBackBuffer(Data::Size size)
{
    META_FUNCTION_TASK();
    return Rhi::BufferSettings{
        Rhi::BufferType::ReadBack,
        Rhi::ResourceUsageMask(Rhi::ResourceUsage::ReadBack),
        size,
        0U,
        PixelFormat::Unknown,
        Rhi::BufferStorageMode::Managed
    };
}

bool BufferSettings::operator==(const BufferSettings& other) const
{
    return std::tie(type, usage_mask, size, item_stride_size, data_format, storage_mode)
        == std::tie(other.type, other.usage_mask, other.size, other.item_stride_size, other.data_format, other.storage_mode);
}

bool BufferSettings::operator!=(const BufferSettings& other) const
{
    return std::tie(type, usage_mask, size, item_stride_size, data_format, storage_mode)
        != std::tie(other.type, other.usage_mask, other.size, other.item_stride_size, other.data_format, other.storage_mode);
}

} // namespace Methane::Graphics::Rhi
