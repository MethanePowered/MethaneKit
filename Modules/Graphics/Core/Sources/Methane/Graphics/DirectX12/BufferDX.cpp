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
#include <Methane/Graphics/BufferFactory.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

static std::vector<D3D12_VERTEX_BUFFER_VIEW> GetNativeVertexBufferViews(const Refs<Buffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertex_buffer_views;
    std::transform(buffer_refs.begin(), buffer_refs.end(), std::back_inserter(vertex_buffer_views),
        [](const Ref<Buffer>& buffer_ref)
        {
           const auto& vertex_buffer = static_cast<const VertexBufferDX&>(buffer_ref.get());
           return vertex_buffer.GetNativeView();
        }
    );
    return vertex_buffer_views;
}

Ptr<Buffer> Buffer::CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Graphics::CreateVertexBuffer<VertexBufferDX>(context, size, stride, is_volatile, stride);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Graphics::CreateIndexBuffer<IndexBufferDX>(context, size, format, is_volatile, format);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Graphics::CreateConstantBuffer<ConstantBufferDX>(context, size, addressable, is_volatile);
}

Ptr<Buffer> Buffer::CreateReadBackBuffer(const IContext& context, Data::Size size)
{
    META_FUNCTION_TASK();
    return Graphics::CreateReadBackBuffer<ReadBackBufferDX>(context, size);
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
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = GetDataSize();
    m_buffer_view.StrideInBytes  = stride;
}

template<>
Opt<Resource::Descriptor> VertexBufferDX::InitializeNativeViewDescriptor(const ViewDX::Id&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

template<>
void IndexBufferDX::InitializeView(PixelFormat format)
{
    META_FUNCTION_TASK();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = GetDataSize();
    m_buffer_view.Format         = TypeConverterDX::PixelFormatToDxgi(format);
}

template<>
Opt<Resource::Descriptor> IndexBufferDX::InitializeNativeViewDescriptor(const ViewDX::Id&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

template<>
void ConstantBufferDX::InitializeView()
{
    META_FUNCTION_TASK();
    const Data::Size data_size   = GetDataSize();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = data_size;
}

template<>
Opt<Resource::Descriptor> ConstantBufferDX::InitializeNativeViewDescriptor(const ViewDX::Id& view_id)
{
    META_FUNCTION_TASK();

    // NOTE: Addressable resources are bound to pipeline using GPU Address and byte offset
    using namespace magic_enum::bitwise_operators;
    if (const Usage usage_mask = GetUsage();
        !static_cast<bool>(usage_mask & Usage::ShaderRead) ||
         static_cast<bool>(usage_mask & Usage::Addressable))
        return std::nullopt;

    const Resource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateConstantBufferView(&m_buffer_view, cpu_descriptor_handle);
    return descriptor;
}

template<>
void ReadBackBufferDX::InitializeView()
{
    META_FUNCTION_TASK();
}

template<>
Opt<Resource::Descriptor> ReadBackBufferDX::InitializeNativeViewDescriptor(const ViewDX::Id&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

Ptr<BufferSet> BufferSet::Create(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetDX>(buffers_type, buffer_refs);
}

BufferSetDX::BufferSetDX(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs)
    : BufferSetBase(buffers_type, buffer_refs)
{
    META_FUNCTION_TASK();
    if (buffers_type == Buffer::Type::Vertex)
    {
        m_vertex_buffer_views = Graphics::GetNativeVertexBufferViews(GetRefs());
    }
}

const std::vector<D3D12_VERTEX_BUFFER_VIEW>& BufferSetDX::GetNativeVertexBufferViews() const
{
    META_FUNCTION_TASK();
    const Buffer::Type buffers_type = GetType();
    META_CHECK_ARG_EQUAL_DESCR(buffers_type, Buffer::Type::Vertex,
                               "unable to get vertex buffer views from buffer of {} type", magic_enum::enum_name(buffers_type));
    return m_vertex_buffer_views;
}

} // namespace Methane::Graphics