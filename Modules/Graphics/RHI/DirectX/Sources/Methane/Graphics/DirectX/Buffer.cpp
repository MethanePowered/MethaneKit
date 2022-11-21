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

FILE: Methane/Graphics/DirectX/Buffer.cpp
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/Buffer.h>
#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/Types.h>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Graphics/Base/BufferFactory.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics::Rhi
{

Ptr<IBuffer> Rhi::IBuffer::CreateVertexBuffer(const Rhi::IContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateVertexBuffer<DirectX::VertexBuffer>(context, size, stride, is_volatile, stride);
}

Ptr<IBuffer> Rhi::IBuffer::CreateIndexBuffer(const Rhi::IContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateIndexBuffer<DirectX::IndexBuffer>(context, size, format, is_volatile, format);
}

Ptr<IBuffer> Rhi::IBuffer::CreateConstantBuffer(const Rhi::IContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateConstantBuffer<DirectX::ConstantBuffer>(context, size, addressable, is_volatile);
}

Ptr<IBuffer> Rhi::IBuffer::CreateReadBackBuffer(const Rhi::IContext& context, Data::Size size)
{
    META_FUNCTION_TASK();
    return Base::CreateReadBackBuffer<DirectX::ReadBackBuffer>(context, size);
}

Data::Size Rhi::IBuffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    // Aligned size must be a multiple 256 bytes
    return (size + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

Ptr<IBufferSet> Rhi::IBufferSet::Create(IBuffer::Type buffers_type, const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<DirectX::BufferSet>(buffers_type, buffer_refs);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::DirectX
{

static std::vector<D3D12_VERTEX_BUFFER_VIEW> GetNativeVertexBufferViews(const Refs<Rhi::IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertex_buffer_views;
    std::transform(buffer_refs.begin(), buffer_refs.end(), std::back_inserter(vertex_buffer_views),
        [](const Ref<Rhi::IBuffer>& buffer_ref)
        {
           const auto& vertex_buffer = static_cast<const VertexBuffer&>(buffer_ref.get());
           return vertex_buffer.GetNativeView();
        }
    );
    return vertex_buffer_views;
}

template<>
void VertexBuffer::InitializeView(Data::Size stride)
{
    META_FUNCTION_TASK();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = GetDataSize();
    m_buffer_view.StrideInBytes  = stride;
}

template<>
Opt<Rhi::IResource::Descriptor> VertexBuffer::InitializeNativeViewDescriptor(const View::Id&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

template<>
void IndexBuffer::InitializeView(PixelFormat format)
{
    META_FUNCTION_TASK();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = GetDataSize();
    m_buffer_view.Format         = TypeConverter::PixelFormatToDxgi(format);
}

template<>
Opt<Rhi::IResource::Descriptor> IndexBuffer::InitializeNativeViewDescriptor(const View::Id&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

template<>
void ConstantBuffer::InitializeView()
{
    META_FUNCTION_TASK();
    const Data::Size data_size   = GetDataSize();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = data_size;
}

template<>
Opt<Rhi::IResource::Descriptor> ConstantBuffer::InitializeNativeViewDescriptor(const View::Id& view_id)
{
    META_FUNCTION_TASK();

    // NOTE: Addressable resources are bound to pipeline using GPU Address and byte offset
    using namespace magic_enum::bitwise_operators;
    if (const Usage usage_mask = GetUsage();
        !usage_mask.shader_read || usage_mask.addressable)
        return std::nullopt;

    const Rhi::IResource::Descriptor& descriptor = GetDescriptorByViewId(view_id);
    const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = GetNativeCpuDescriptorHandle(descriptor);
    GetDirectContext().GetDirectDevice().GetNativeDevice()->CreateConstantBufferView(&m_buffer_view, cpu_descriptor_handle);
    return descriptor;
}

template<>
void ReadBackBuffer::InitializeView()
{
    META_FUNCTION_TASK();
}

template<>
Opt<Rhi::IResource::Descriptor> ReadBackBuffer::InitializeNativeViewDescriptor(const View::Id&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

BufferSet::BufferSet(Rhi::IBuffer::Type buffers_type, const Refs<Rhi::IBuffer>& buffer_refs)
    : Base::BufferSet(buffers_type, buffer_refs)
{
    META_FUNCTION_TASK();
    if (buffers_type == Rhi::IBuffer::Type::Vertex)
    {
        m_vertex_buffer_views = DirectX::GetNativeVertexBufferViews(GetRefs());
    }
}

const std::vector<D3D12_VERTEX_BUFFER_VIEW>& BufferSet::GetNativeVertexBufferViews() const
{
    META_FUNCTION_TASK();
    const Rhi::IBuffer::Type buffers_type = GetType();
    META_CHECK_ARG_EQUAL_DESCR(buffers_type, Rhi::IBuffer::Type::Vertex,
                               "unable to get vertex buffer views from buffer of {} type", magic_enum::enum_name(buffers_type));
    return m_vertex_buffer_views;
}

} // namespace Methane::Graphics