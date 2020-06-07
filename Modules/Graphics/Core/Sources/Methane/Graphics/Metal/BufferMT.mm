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
#import "../../../../../../Data/Types/Include/Methane/Data/Types.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

Ptr<Buffer> Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Vertex, Usage::Unknown, size, stride, PixelFormat::Unknown };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Index, Usage::Unknown, size, GetPixelSize(format), format };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    Usage::Mask usage_mask = Usage::ShaderRead;
    if (addressable)
        usage_mask |= Usage::Addressable;

    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0u, PixelFormat::Unknown };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

BufferMT::BufferMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : BufferBase(context, settings, descriptor_by_usage)
    , m_mtl_buffer([GetContextMT().GetDeviceMT().GetNativeDevice() newBufferWithLength:settings.size options:MTLResourceStorageModeManaged])
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
}

BufferMT::~BufferMT()
{
    META_FUNCTION_TASK();
    GetContextBase().GetResourceManager().GetReleasePool().AddResource(*this);
}

void BufferMT::SetName(const std::string& name)
{
    META_FUNCTION_TASK();

    BufferBase::SetName(name);
    m_mtl_buffer.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
}

void BufferMT::SetData(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();

    BufferBase::SetData(sub_resources);

    assert(m_mtl_buffer != nil);
    Data::RawPtr p_resource_data = static_cast<Data::RawPtr>([m_mtl_buffer contents]);
    assert(p_resource_data != nullptr);

    Data::Size data_size = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        std::copy(sub_resource.p_data, sub_resource.p_data + sub_resource.size, p_resource_data + data_size);
        data_size += sub_resource.size;
    }

    if (m_mtl_buffer.storageMode == MTLStorageModeManaged)
    {
        [m_mtl_buffer didModifyRange:NSMakeRange(0, data_size)];
    }
}

MTLIndexType BufferMT::GetNativeIndexType() const noexcept
{
    META_FUNCTION_TASK();
    return TypeConverterMT::DataFormatToMetalIndexType(GetSettings().data_format);
}

Ptr<Buffers> Buffers::Create(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BuffersMT>(buffers_type, std::move(buffer_refs));
}

BuffersMT::BuffersMT(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
    : BuffersBase(buffers_type, std::move(buffer_refs))
    , m_mtl_buffer_offsets(GetCount(), 0u)
{
    META_FUNCTION_TASK();
    const Refs<Buffer>& refs = GetRefs();
    m_mtl_buffers.reserve(refs.size());
    std::transform(refs.begin(), refs.end(), std::back_inserter(m_mtl_buffers),
        [](const Ref<Buffer>& buffer_ref)
        {
           const BufferMT& metal_buffer = static_cast<const BufferMT&>(buffer_ref.get());
           return metal_buffer.GetNativeBuffer();
        }
    );
}

} // namespace Methane::Graphics
