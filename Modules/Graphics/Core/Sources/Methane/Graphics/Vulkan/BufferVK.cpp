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

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <iterator>

namespace Methane::Graphics
{

Ptr<Buffer> Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Vertex, Usage::Unknown, size, stride, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Index, Usage::Unknown, size, GetPixelSize(format), format, Buffer::StorageMode::Private };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const Usage::Mask usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::Unknown);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Buffer> Buffer::CreateVolatileBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    const Usage::Mask usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::Unknown);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Managed };
    return std::make_shared<BufferVK>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

BufferVK::BufferVK(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : BufferBase(context, settings, descriptor_by_usage)
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
}

BufferVK::~BufferVK()
{
    META_FUNCTION_TASK();
}

void BufferVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    BufferBase::SetName(name);
}

void BufferVK::SetData(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();

    BufferBase::SetData(sub_resources);
}

Ptr<BufferSet> BufferSet::Create(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetVK>(buffers_type, std::move(buffer_refs));
}

BufferSetVK::BufferSetVK(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
    : BufferSetBase(buffers_type, std::move(buffer_refs))
{
    META_FUNCTION_TASK();
    switch(buffers_type)
    {
    case Buffer::Type::Vertex: break;
    default: break;
    }
}

} // namespace Methane::Graphics
