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

FILE: Methane/Graphics/DirectX12/BufferDX.cpp
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#include "BufferDX.h"
#include "ContextDX.h"
#include "DeviceDX.h"
#include "TypesDX.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Buffer::Ptr Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Vertex, Usage::Unknown, size };
    return std::make_shared<VertexBufferDX>(static_cast<ContextBase&>(context), settings, DescriptorByUsage(), stride);
}

Buffer::Ptr Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Index, Usage::Unknown, size };
    return std::make_shared<IndexBufferDX>(static_cast<ContextBase&>(context), settings, DescriptorByUsage(), format);
}

Buffer::Ptr Buffer::CreateConstantBuffer(Context& context, Data::Size size, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Constant, Usage::ShaderRead, size };
    return std::make_shared<ConstantBufferDX>(static_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    ITT_FUNCTION_TASK();
    // Aligned size must be a multiple 256 bytes
    return (size + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

template<>
void VertexBufferDX::InitializeView(Data::Size stride)
{
    ITT_FUNCTION_TASK();

    const Data::Size data_size     = GetDataSize();
    m_buffer_view.BufferLocation   = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes      = static_cast<UINT>(data_size);
    m_buffer_view.StrideInBytes    = static_cast<UINT>(stride);
    m_formatted_items_count        = stride > 0 ? data_size / stride : 0;
}

template<>
void IndexBufferDX::InitializeView(PixelFormat format)
{
    ITT_FUNCTION_TASK();

    const Data::Size data_size     = GetDataSize();
    const Data::Size element_size  = GetPixelSize(format);
    m_buffer_view.BufferLocation   = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes      = static_cast<UINT>(data_size);
    m_buffer_view.Format           = TypeConverterDX::DataFormatToDXGI(format);
    m_formatted_items_count        = element_size > 0 ? data_size / element_size : 0;
}

template<>
void ConstantBufferDX::InitializeView()
{
    ITT_FUNCTION_TASK();

    const Data::Size data_size     = GetDataSize();
    m_buffer_view.BufferLocation = GetNativeGpuAddress();
    m_buffer_view.SizeInBytes    = static_cast<UINT>(data_size);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = GetNativeCPUDescriptorHandle(Usage::ShaderRead);
    GetContextDX().GetDeviceDX().GetNativeDevice()->CreateConstantBufferView(&m_buffer_view, cpu_handle);
}

} // namespace Methane::Graphics