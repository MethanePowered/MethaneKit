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

FILE: Methane/Graphics/Metal/BufferMT.mm
Metal implementation of the buffer interface.

******************************************************************************/

#include "BufferMT.hh"
#include "DeviceMT.hh"
#include "ContextMT.h"
#include "TypesMT.hh"
#include "BlitCommandListMT.hh"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

static MTLResourceOptions GetNativeResourceOptions(Buffer::StorageMode storage_mode)
{
    switch(storage_mode)
    {
    case Buffer::StorageMode::Managed: return MTLResourceStorageModeManaged;
    case Buffer::StorageMode::Private: return MTLResourceStorageModePrivate;
    }
    assert(0);
    return MTLResourceStorageModeShared;
}

Ptr<Buffer> Buffer::CreateVertexBuffer(Context& context, Data::Size size, Data::Size stride)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Vertex, Usage::Unknown, size, stride, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(Context& context, Data::Size size, PixelFormat format)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Index, Usage::Unknown, size, GetPixelSize(format), format, Buffer::StorageMode::Private };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    const Usage::Mask usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::Unknown);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Buffer> Buffer::CreateVolatileBuffer(Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    const Usage::Mask usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::Unknown);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Managed };
    return std::make_shared<BufferMT>(dynamic_cast<ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

BufferMT::BufferMT(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : BufferBase(context, settings, descriptor_by_usage)
    , m_mtl_buffer([GetContextMT().GetDeviceMT().GetNativeDevice() newBufferWithLength:settings.size
                                                                               options:GetNativeResourceOptions(settings.storage_mode)])
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
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

    switch(GetSettings().storage_mode)
    {
    case Buffer::StorageMode::Managed: SetDataToManagedBuffer(sub_resources); break;
    case Buffer::StorageMode::Private: SetDataToPrivateBuffer(sub_resources); break;
    }
}

void BufferMT::SetDataToManagedBuffer(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    assert(GetSettings().storage_mode == Buffer::StorageMode::Managed);
    assert(m_mtl_buffer != nil);
    assert(m_mtl_buffer.storageMode == MTLStorageModeManaged);

    Data::RawPtr p_resource_data = static_cast<Data::RawPtr>([m_mtl_buffer contents]);
    assert(p_resource_data != nullptr);

    Data::Size data_offset = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        if (sub_resource.data_range)
            data_offset = sub_resource.data_range->GetStart();

        std::copy(sub_resource.p_data, sub_resource.p_data + sub_resource.size, p_resource_data + data_offset);

        [m_mtl_buffer didModifyRange:NSMakeRange(data_offset, data_offset + sub_resource.size)];
        data_offset += sub_resource.size;
    }
}

void BufferMT::SetDataToPrivateBuffer(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    assert(GetSettings().storage_mode == Buffer::StorageMode::Private);
    assert(m_mtl_buffer != nil);
    assert(m_mtl_buffer.storageMode == MTLStorageModePrivate);

    id<MTLDevice>& mtl_device = GetContextMT().GetDeviceMT().GetNativeDevice();

    BlitCommandListMT& blit_command_list = static_cast<BlitCommandListMT&>(GetContextBase().GetUploadCommandList());
    blit_command_list.RetainResource(*this);

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = blit_command_list.GetNativeCommandEncoder();
    assert(mtl_blit_encoder != nil);

    Data::Size data_offset = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        // Create temporary buffer with shared storage mode for sub-resource data upload to private buffer on GPU
        id <MTLBuffer> mtl_sub_resource_upload_buffer = [mtl_device newBufferWithBytes:sub_resource.p_data
                                                                                length:sub_resource.size
                                                                               options:MTLResourceStorageModeShared];

        if (sub_resource.data_range)
            data_offset = sub_resource.data_range->GetStart();

        [mtl_blit_encoder copyFromBuffer:mtl_sub_resource_upload_buffer
                            sourceOffset:0
                                toBuffer:m_mtl_buffer
                       destinationOffset:data_offset
                                    size:sub_resource.size];

        data_offset += sub_resource.size;
    }

    GetContextBase().RequestDeferredAction(Context::DeferredAction::UploadResources);
}

MTLIndexType BufferMT::GetNativeIndexType() const noexcept
{
    META_FUNCTION_TASK();
    return TypeConverterMT::DataFormatToMetalIndexType(GetSettings().data_format);
}

Ptr<BufferSet> BufferSet::Create(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetMT>(buffers_type, std::move(buffer_refs));
}

BufferSetMT::BufferSetMT(Buffer::Type buffers_type, Refs<Buffer> buffer_refs)
    : BufferSetBase(buffers_type, std::move(buffer_refs))
    , m_mtl_buffer_offsets(GetCount(), 0U)
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
