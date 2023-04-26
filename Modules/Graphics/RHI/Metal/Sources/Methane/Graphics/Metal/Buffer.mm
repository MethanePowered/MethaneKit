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

FILE: Methane/Graphics/Metal/Buffer.mm
Metal implementation of the buffer interface.

******************************************************************************/

#include <Methane/Graphics/Metal/Buffer.hh>
#include <Methane/Graphics/Metal/IContext.h>
#include <Methane/Graphics/Metal/Types.hh>
#include <Methane/Graphics/Metal/TransferCommandList.hh>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics::Metal
{

#ifdef APPLE_MACOS
#define NativeResourceStorageModeManaged MTLResourceStorageModeManaged
#define NativeStorageModeManaged MTLStorageModeManaged
#else
#define NativeResourceStorageModeManaged MTLResourceStorageModeShared
#define NativeStorageModeManaged MTLStorageModeShared
#endif

static MTLResourceOptions GetNativeResourceOptions(Rhi::BufferStorageMode storage_mode)
{
    switch(storage_mode)
    {
    case Rhi::BufferStorageMode::Managed: return NativeResourceStorageModeManaged;
    case Rhi::BufferStorageMode::Private: return MTLResourceStorageModePrivate;
    default: META_UNEXPECTED_ARG_RETURN(storage_mode, MTLResourceStorageModeShared);
    }
}

Buffer::Buffer(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
    , m_mtl_buffer([GetMetalContext().GetMetalDevice().GetNativeDevice() newBufferWithLength:settings.size
                                                                                     options:GetNativeResourceOptions(settings.storage_mode)])
{ }

bool Buffer::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    m_mtl_buffer.label = MacOS::ConvertToNsString(name);
    return true;
}

void Buffer::SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResource& sub_resource)
{
    META_FUNCTION_TASK();
    Base::Buffer::SetData(target_cmd_queue, sub_resource);

    switch(GetSettings().storage_mode)
    {
    case IBuffer::StorageMode::Managed: SetDataToManagedBuffer(sub_resource); break;
    case IBuffer::StorageMode::Private: SetDataToPrivateBuffer(sub_resource); break;
    default: META_UNEXPECTED_ARG(GetSettings().storage_mode);
    }
}

Rhi::SubResource Buffer::GetData(Rhi::ICommandQueue&, const BytesRangeOpt& data_range)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(GetUsage().HasAnyBit(Rhi::ResourceUsage::ReadBack),
                              "getting buffer data from GPU is allowed for buffers with CPU Read-back flag only");

    const BytesRange buffer_data_range(data_range ? data_range->GetStart() : 0U,
                                       data_range ? data_range->GetEnd()   : GetDataSize());

    Data::Bytes data;
    switch(GetSettings().storage_mode)
    {
    case IBuffer::StorageMode::Managed: data = GetDataFromManagedBuffer(buffer_data_range); break;
    case IBuffer::StorageMode::Private: data = GetDataFromPrivateBuffer(buffer_data_range); break;
    default: META_UNEXPECTED_ARG_RETURN(GetSettings().storage_mode, SubResource());
    }

    return Rhi::SubResource(std::move(data), Rhi::SubResourceIndex(), data_range);
}

void Buffer::SetDataToManagedBuffer(const SubResource& sub_resource)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetSettings().storage_mode, IBuffer::StorageMode::Managed);
    META_CHECK_ARG_NOT_NULL(m_mtl_buffer);
    META_CHECK_ARG_EQUAL(m_mtl_buffer.storageMode, NativeStorageModeManaged);

    Data::RawPtr resource_data_ptr = static_cast<Data::RawPtr>([m_mtl_buffer contents]);
    META_CHECK_ARG_NOT_NULL(resource_data_ptr);

    Data::Size data_offset = 0;
    if (sub_resource.HasDataRange())
        data_offset = sub_resource.GetDataRange().GetStart();

    std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), resource_data_ptr + data_offset);

#ifdef APPLE_MACOS // storage_mode == MTLStorageModeManaged
    [m_mtl_buffer didModifyRange:NSMakeRange(data_offset, data_offset + sub_resource.GetDataSize())];
#endif
}

void Buffer::SetDataToPrivateBuffer(const SubResource& sub_resource)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetSettings().storage_mode, IBuffer::StorageMode::Private);
    META_CHECK_ARG_NOT_NULL(m_mtl_buffer);
    META_CHECK_ARG_EQUAL(m_mtl_buffer.storageMode, MTLStorageModePrivate);

    TransferCommandList& transfer_command_list = dynamic_cast<TransferCommandList&>(GetBaseContext().GetUploadCommandKit().GetListForEncoding());
    transfer_command_list.RetainResource(*this);

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = transfer_command_list.GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_blit_encoder);

    Data::Size data_offset = 0;
    if (sub_resource.HasDataRange())
        data_offset = sub_resource.GetDataRange().GetStart();

    [mtl_blit_encoder copyFromBuffer:GetUploadSubresourceBuffer(sub_resource, Rhi::SubResourceCount())
                        sourceOffset:0U
                            toBuffer:m_mtl_buffer
                   destinationOffset:data_offset
                                size:sub_resource.GetDataSize()];

    GetBaseContext().RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources);
}

Data::Bytes Buffer::GetDataFromManagedBuffer(const BytesRange& data_range)
{
    META_FUNCTION_TASK();
    auto* data_ptr = reinterpret_cast<std::byte*>([m_mtl_buffer contents]);
    Data::Size data_size = static_cast<Data::Size>([m_mtl_buffer length]);

    META_CHECK_ARG_LESS_DESCR(data_range.GetEnd(), data_size, "provided subresource data range is out of buffer bounds");
    data_ptr += data_range.GetStart();

    return Data::Bytes(data_ptr, data_ptr + data_range.GetLength());
}

Data::Bytes Buffer::GetDataFromPrivateBuffer(const BytesRange& data_range)
{
    META_FUNCTION_TASK();
    TransferCommandList& transfer_command_list = dynamic_cast<TransferCommandList&>(GetBaseContext().GetUploadCommandKit().GetListForEncoding());
    transfer_command_list.RetainResource(*this);

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = transfer_command_list.GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_blit_encoder);

    const id<MTLBuffer> mtl_read_back_buffer = GetReadBackBuffer(data_range.GetLength());
    [mtl_blit_encoder copyFromBuffer:m_mtl_buffer
                        sourceOffset:data_range.GetStart()
                            toBuffer:mtl_read_back_buffer
                   destinationOffset:0U
                                size:data_range.GetLength()];

    GetBaseContext().UploadResources();

    auto* data_ptr = reinterpret_cast<std::byte*>([mtl_read_back_buffer contents]) + data_range.GetStart();
    return Data::Bytes(data_ptr, data_ptr + data_range.GetLength());
}

MTLIndexType Buffer::GetNativeIndexType() const noexcept
{
    META_FUNCTION_TASK();
    return TypeConverter::DataFormatToMetalIndexType(GetSettings().data_format);
}

} // namespace Methane::Graphics::Metal
