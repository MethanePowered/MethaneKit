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

FILE: Methane/Graphics/Metal/BufferMT.mm
Metal implementation of the buffer interface.

******************************************************************************/

#include "BufferMT.hh"
#include "DeviceMT.hh"
#include "ContextMT.h"
#include "TypesMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

#include <algorithm>
#include <iterator>
#include <cassert>

namespace Methane::Graphics
{

Ptr<Buffer> Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Vertex, Usage::Unknown, size };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings, stride, PixelFormat::Unknown);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Index, Usage::Unknown, size };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings, 0, format);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    Usage::Mask usage_mask = Usage::ShaderRead;
    if (addressable)
        usage_mask |= Usage::Addressable;

    const Buffer::Settings settings = { Buffer::Type::Constant, usage_mask, size };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
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
    GetContext().GetResourceManager().GetReleasePool().AddResource(*this);
}

void BufferMT::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    BufferBase::SetName(name);
    m_mtl_buffer.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
}

void BufferMT::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();

    BufferBase::SetData(sub_resources);

    assert(m_mtl_buffer != nil);
    Data::RawPtr p_resource_data = static_cast<Data::RawPtr>([m_mtl_buffer contents]);
    assert(p_resource_data != nullptr);

    Data::Size data_size = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        std::copy(sub_resource.p_data, sub_resource.p_data + sub_resource.data_size, p_resource_data + data_size);
        data_size += sub_resource.data_size;
    }

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

} // namespace Methane::Graphics
