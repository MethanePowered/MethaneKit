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

namespace Methane::Graphics::Rhi
{

Ptr<IBuffer> IBuffer::Create(const IContext& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::Buffer>(dynamic_cast<const Base::Context&>(context), settings);
}

} // namespace Methane::Graphics::Rhi

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
{
    META_FUNCTION_TASK();
}

bool Buffer::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    m_mtl_buffer.label = MacOS::ConvertToNsString(name);
    return true;
}

void Buffer::SetData(const SubResources& sub_resources, Rhi::ICommandQueue& target_cmd_queue)
{
    META_FUNCTION_TASK();
    Resource::SetData(sub_resources, target_cmd_queue);

    switch(GetSettings().storage_mode)
    {
    case IBuffer::StorageMode::Managed: SetDataToManagedBuffer(sub_resources); break;
    case IBuffer::StorageMode::Private: SetDataToPrivateBuffer(sub_resources); break;
    default: META_UNEXPECTED_ARG(GetSettings().storage_mode);
    }
}

void Buffer::SetDataToManagedBuffer(const SubResources& sub_resources)
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

void Buffer::SetDataToPrivateBuffer(const SubResources& sub_resources)
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
    
    GetBaseContext().RequestDeferredAction(Rhi::ContextDeferredAction::UploadResources);
}

MTLIndexType Buffer::GetNativeIndexType() const noexcept
{
    META_FUNCTION_TASK();
    return TypeConverter::DataFormatToMetalIndexType(GetSettings().data_format);
}

} // namespace Methane::Graphics::Metal
