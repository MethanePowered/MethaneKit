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

#include <Methane/Graphics/BufferMT.hh>
#include <Methane/Graphics/DeviceMT.hh>
#include <Methane/Graphics/ContextMT.h>
#include <Methane/Graphics/TypesMT.hh>
#include <Methane/Graphics/TransferCommandListMT.hh>

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/ICommandKit.h>
#include <Methane/Graphics/Base/BufferFactory.hpp>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

#ifdef APPLE_MACOS
#define NativeResourceStorageModeManaged MTLResourceStorageModeManaged
#define NativeStorageModeManaged MTLStorageModeManaged
#else
#define NativeResourceStorageModeManaged MTLResourceStorageModeShared
#define NativeStorageModeManaged MTLStorageModeShared
#endif

static MTLResourceOptions GetNativeResourceOptions(IBuffer::StorageMode storage_mode)
{
    switch(storage_mode)
    {
    case IBuffer::StorageMode::Managed: return NativeResourceStorageModeManaged;
    case IBuffer::StorageMode::Private: return MTLResourceStorageModePrivate;
    default: META_UNEXPECTED_ARG_RETURN(storage_mode, MTLResourceStorageModeShared);
    }
}

Ptr<IBuffer> IBuffer::CreateVertexBuffer(const IContext& context, Data::Size size, Data::Size stride, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateVertexBuffer<BufferMT>(context, size, stride, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateIndexBuffer(const IContext& context, Data::Size size, PixelFormat format, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateIndexBuffer<BufferMT>(context, size, format, is_volatile);
}

Ptr<IBuffer> IBuffer::CreateConstantBuffer(const IContext& context, Data::Size size, bool addressable, bool is_volatile)
{
    META_FUNCTION_TASK();
    return Base::CreateConstantBuffer<BufferMT>(context, size, addressable, is_volatile);
}

Data::Size IBuffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

BufferMT::BufferMT(const Base::Context& context, const Settings& settings)
    : ResourceMT(context, settings)
    , m_mtl_buffer([GetContextMT().GetDeviceMT().GetNativeDevice() newBufferWithLength:settings.size
                                                                               options:GetNativeResourceOptions(settings.storage_mode)])
{
    META_FUNCTION_TASK();
}

bool BufferMT::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (!ResourceMT::SetName(name))
        return false;

    m_mtl_buffer.label = MacOS::ConvertToNsType<std::string, NSString*>(name);
    return true;
}

void BufferMT::SetData(const SubResources& sub_resources, ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    ResourceMT::SetData(sub_resources, target_cmd_queue);

    switch(GetSettings().storage_mode)
    {
    case IBuffer::StorageMode::Managed: SetDataToManagedBuffer(sub_resources); break;
    case IBuffer::StorageMode::Private: SetDataToPrivateBuffer(sub_resources); break;
    default: META_UNEXPECTED_ARG(GetSettings().storage_mode);
    }
}

void BufferMT::SetDataToManagedBuffer(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetSettings().storage_mode, IBuffer::StorageMode::Managed);
    META_CHECK_ARG_NOT_NULL(m_mtl_buffer);
    META_CHECK_ARG_EQUAL(m_mtl_buffer.storageMode, NativeStorageModeManaged);

    Data::RawPtr p_resource_data = static_cast<Data::RawPtr>([m_mtl_buffer contents]);
    META_CHECK_ARG_NOT_NULL(p_resource_data);

    Data::Size data_offset = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        if (sub_resource.HasDataRange())
            data_offset = sub_resource.GetDataRange().GetStart();

        std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), p_resource_data + data_offset);

#ifdef APPLE_MACOS // storage_mode == MTLStorageModeManaged
        [m_mtl_buffer didModifyRange:NSMakeRange(data_offset, data_offset + sub_resource.GetDataSize())];
#endif
        data_offset += sub_resource.GetDataSize();
    }
}

void BufferMT::SetDataToPrivateBuffer(const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_EQUAL(GetSettings().storage_mode, IBuffer::StorageMode::Private);
    META_CHECK_ARG_NOT_NULL(m_mtl_buffer);
    META_CHECK_ARG_EQUAL(m_mtl_buffer.storageMode, MTLStorageModePrivate);

    TransferCommandListMT& transfer_command_list = dynamic_cast<TransferCommandListMT&>(GetContextBase().GetUploadCommandKit().GetListForEncoding());
    transfer_command_list.RetainResource(*this);

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = transfer_command_list.GetNativeCommandEncoder();
    META_CHECK_ARG_NOT_NULL(mtl_blit_encoder);

    Data::Size data_offset = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        if (sub_resource.HasDataRange())
            data_offset = sub_resource.GetDataRange().GetStart();

        [mtl_blit_encoder copyFromBuffer:GetUploadSubresourceBuffer(sub_resource)
                            sourceOffset:0
                                toBuffer:m_mtl_buffer
                       destinationOffset:data_offset
                                    size:sub_resource.GetDataSize()];

        data_offset += sub_resource.GetDataSize();
    }
    
    GetContextBase().RequestDeferredAction(ContextDeferredAction::UploadResources);
}

MTLIndexType BufferMT::GetNativeIndexType() const noexcept
{
    META_FUNCTION_TASK();
    return TypeConverterMT::DataFormatToMetalIndexType(GetSettings().data_format);
}

Ptr<IBufferSet> IBufferSet::Create(IBuffer::Type buffers_type, const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetMT>(buffers_type, buffer_refs);
}

BufferSetMT::BufferSetMT(IBuffer::Type buffers_type, const Refs<IBuffer>& buffer_refs)
    : Base::BufferSet(buffers_type, buffer_refs)
    , m_mtl_buffer_offsets(GetCount(), 0U)
{
    META_FUNCTION_TASK();
    const Refs<IBuffer>& refs = GetRefs();
    m_mtl_buffers.reserve(refs.size());
    std::transform(refs.begin(), refs.end(), std::back_inserter(m_mtl_buffers),
        [](const Ref<IBuffer>& buffer_ref)
        {
           const BufferMT& metal_buffer = static_cast<const BufferMT&>(buffer_ref.get());
           return metal_buffer.GetNativeBuffer();
        }
    );
}

} // namespace Methane::Graphics
