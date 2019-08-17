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

FILE: Methane/Graphics/Metal/BufferMT.mm
Metal implementation of the buffer interface.

******************************************************************************/

#include "BufferMT.hh"
#include "DeviceMT.hh"
#include "ContextMT.hh"
#include "TypesMT.hh"

#include <Methane/Graphics/Instrumentation.h>
#include <Methane/Platform/MacOS/Types.hh>

#include <algorithm>
#include <iterator>
#include <cassert>

using namespace Methane;
using namespace Methane::Graphics;

Buffer::Ptr Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Vertex, Usage::Unknown, size };
    return std::make_shared<BufferMT>(static_cast<ContextBase&>(context), settings, stride, PixelFormat::Unknown);
}

Buffer::Ptr Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Index, Usage::Unknown, size };
    return std::make_shared<BufferMT>(static_cast<ContextBase&>(context), settings, 0, format);
}

Buffer::Ptr Buffer::CreateConstantBuffer(Context& context, Data::Size size, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Constant, Usage::Unknown, size };
    return std::make_shared<BufferMT>(static_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    ITT_FUNCTION_TASK();
    return size;
}

BufferMT::BufferMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : BufferBase(context, settings, descriptor_by_usage)
    , m_mtl_buffer([GetContextMT().GetDeviceMT().GetNativeDevice() newBufferWithLength:settings.size options:MTLResourceStorageModeManaged])
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
}

BufferMT::BufferMT(ContextBase& context, const Settings& settings, Data::Size stride, PixelFormat format, const DescriptorByUsage& descriptor_by_usage)
    : BufferMT(context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    m_stride = stride;
    m_format = format;
}

BufferMT::~BufferMT()
{
    ITT_FUNCTION_TASK();
    m_context.GetResourceManager().GetReleasePool().AddResource(*this);
}

void BufferMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    BufferBase::SetName(name);
    m_mtl_buffer.label = MacOS::ConvertToNSType<std::string, NSString*>(name);
}

void BufferMT::SetData(Data::ConstRawPtr p_data, Data::Size data_size)
{
    ITT_FUNCTION_TASK();

    BufferBase::SetData(p_data, data_size);

    assert(m_mtl_buffer != nil);
    Data::RawPtr p_resource_data = static_cast<Data::RawPtr>([m_mtl_buffer contents]);

    assert(!!p_resource_data);
    std::copy(p_data, p_data + data_size, p_resource_data);

    if (m_mtl_buffer.storageMode == MTLStorageModeManaged)
    {
        [m_mtl_buffer didModifyRange:NSMakeRange(0, data_size)];
    }
}

uint32_t BufferMT::GetFormattedItemsCount() const
{
    ITT_FUNCTION_TASK();

    const Data::Size data_size = GetDataSize();
    if (m_stride > 0)
    {
        return data_size / m_stride;
    }
    else
    {
        const Data::Size element_size = GetPixelSize(m_format);
        return element_size > 0 ? data_size / element_size : 0;
    }
}

MTLIndexType BufferMT::GetNativeIndexType() const noexcept
{
    ITT_FUNCTION_TASK();
    return TypeConverterMT::DataFormatToMetalIndexType(m_format);
}
