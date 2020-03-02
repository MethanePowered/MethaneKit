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

FILE: Methane/Graphics/Vulkan/BufferVK.mm
Vulkan implementation of the buffer interface.

******************************************************************************/

#include "BufferVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <iterator>

namespace Methane::Graphics
{

Ptr<Buffer> Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Vertex, Usage::Unknown, size };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings, stride, PixelFormat::Unknown);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    ITT_FUNCTION_TASK();
    const Buffer::Settings settings = { Buffer::Type::Index, Usage::Unknown, size };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings, 0, format);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    ITT_FUNCTION_TASK();
    Usage::Mask usage_mask = Usage::ShaderRead;
    if (addressable)
        usage_mask |= Usage::Addressable;

    const Buffer::Settings settings = { Buffer::Type::Constant, usage_mask, size };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    ITT_FUNCTION_TASK();
    return size;
}

BufferVK::BufferVK(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : BufferBase(context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    InitializeDefaultDescriptors();
}

BufferVK::BufferVK(ContextBase& context, const Settings& settings, Data::Size stride, PixelFormat format, const DescriptorByUsage& descriptor_by_usage)
    : BufferVK(context, settings, descriptor_by_usage)
{
    ITT_FUNCTION_TASK();

    m_stride = stride;
    m_format = format;
}

BufferVK::~BufferVK()
{
    ITT_FUNCTION_TASK();
    GetContext().GetResourceManager().GetReleasePool().AddResource(*this);
}

void BufferVK::SetName(const std::string& name)
{
    ITT_FUNCTION_TASK();

    BufferBase::SetName(name);
}

void BufferVK::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();

    BufferBase::SetData(sub_resources);
}

uint32_t BufferVK::GetFormattedItemsCount() const
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

} // namespace Methane::Graphics
