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

FILE: Methane/Graphics/DirectX12/BufferDX.cpp
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#include "BufferDX.h"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

static std::vector<D3D12_VERTEX_BUFFER_VIEW> GetNativeVertexBufferViews(const Refs<Buffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertex_buffer_views;
    std::transform(buffer_refs.begin(), buffer_refs.end(), std::back_inserter(vertex_buffer_views),
        [](const Ref<Buffer>& buffer_ref)
        {
           const VertexBufferDX& vertex_buffer = static_cast<const VertexBufferDX&>(buffer_ref.get());
           return vertex_buffer.GetNativeView();
        }
    );
    return vertex_buffer_views;
}

Ptr<Buffer> Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Vertex, Usage::Unknown, size, stride, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<VertexBufferDX>(dynamic_cast<ContextBase&>(context), settings, DescriptorByUsage(), stride);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Index, Usage::Unknown, size, GetPixelSize(format), format, Buffer::StorageMode::Private };
    return std::make_shared<IndexBufferDX>(dynamic_cast<ContextBase&>(context), settings, DescriptorByUsage(), format);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();

    const Usage::Mask usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::Unknown);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<ConstantBufferDX>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Buffer> Buffer::CreateVolatileBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    const Usage::Mask usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::Unknown);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Managed };
    return std::make_shared<ConstantBufferDX>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Buffer> Buffer::CreateReadBackBuffer(Context& context, Data::Size size)
{
    const Buffer::Settings settings{ Buffer::Type::ReadBack, Usage::ReadBack, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Managed };
    return std::make_shared<ReadBackBufferDX>(dynamic_cast<ContextBase&>(context), settings, DescriptorByUsage());
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    // Aligned size must be a multiple 256 bytes
    return (size + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

template<>
void VertexBufferDX::InitializeView(Data::Size stride)
{
    META_FUNCTION_TASK();

    m_buffer_view.BufferLocation   = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes      = static_cast<UINT>(GetDataSize());
    m_buffer_view.StrideInBytes    = static_cast<UINT>(stride);
}

template<>
void IndexBufferDX::InitializeView(PixelFormat format)
{
    META_FUNCTION_TASK();

    m_buffer_view.BufferLocation   = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes      = static_cast<UINT>(GetDataSize());
    m_buffer_view.Format           = TypeConverterDX::PixelFormatToDxgi(format);
}

template<>
void ConstantBufferDX::InitializeView()
{
    META_FUNCTION_TASK();

    const Data::Size data_size   = GetDataSize();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = static_cast<UINT>(data_size);

    // NOTE: Addressable resources are bound to pipeline using GPU Address and byte offset
    const Usage::Mask usage_mask = GetUsageMask();
    if (usage_mask & Usage::ShaderRead && !(usage_mask & Usage::Addressable))
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = GetNativeCpuDescriptorHandle(Usage::ShaderRead);
        GetContextDX().GetDeviceDX().GetNativeDevice()->CreateConstantBufferView(&m_buffer_view, cpu_handle);
    }
}

template<>
void ReadBackBufferDX::InitializeView()
{
    META_FUNCTION_TASK();
}

Ptr<BufferSet> BufferSet::Create(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetDX>(buffers_type, std::move(buffer_refs));
}

BufferSetDX::BufferSetDX(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
    : BufferSetBase(buffers_type, std::move(buffer_refs))
{
    META_FUNCTION_TASK();
    switch(buffers_type)
    {
    case Buffer::Type::Vertex: m_vertex_buffer_views = Graphics::GetNativeVertexBufferViews(GetRefs()); break;
    default: break;
    }
}

const std::vector<D3D12_VERTEX_BUFFER_VIEW>& BufferSetDX::GetNativeVertexBufferViews() const
{
    META_FUNCTION_TASK();
    const Buffer::Type buffers_type = GetType();
    if (buffers_type != Buffer::Type::Vertex)
        throw std::logic_error("Unable to get vertex buffer views from buffer of \"" + Buffer::GetBufferTypeName(buffers_type) + "\" type.");

    return m_vertex_buffer_views;
}

} // namespace Methane::Graphics