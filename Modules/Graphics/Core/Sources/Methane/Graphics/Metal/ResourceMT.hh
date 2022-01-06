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

FILE: Methane/Graphics/Metal/ResourceMT.hh
Metal implementation of the resource interface.

******************************************************************************/
#pragma once

#include "DeviceMT.hh"

#include <Methane/Graphics/ResourceBase.h>
#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Instrumentation.h>

#include <vector>

namespace Methane::Graphics
{

struct IContextMT;

template<typename ReourceBaseType, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, ReourceBaseType>, void>>
class ResourceMT : public ReourceBaseType
{
public:
    template<typename SettingsType>
    ResourceMT(const ContextBase& context, const SettingsType& settings, const Resource::DescriptorByUsage&)
        : ReourceBaseType(context, settings)
    {
        META_FUNCTION_TASK();
    }

    ~ResourceMT() override
    {
        META_FUNCTION_TASK();
        // Resource released callback has to be emitted before native resource is released
        Data::Emitter<IResourceCallback>::Emit(&IResourceCallback::OnResourceReleased, std::ref(*this));
    }

    // IResource overrides
    const Resource::DescriptorByUsage& GetDescriptorByUsage() const noexcept final
    {
        META_FUNCTION_TASK();
        static const Resource::DescriptorByUsage descriptor_by_usage;
        return descriptor_by_usage;
    }

    const Resource::Descriptor& GetDescriptor(Resource::Usage) const final
    {
        META_FUNCTION_NOT_IMPLEMENTED();
    }

protected:
    const IContextMT& GetContextMT() const noexcept
    {
        META_FUNCTION_TASK();
        return static_cast<const IContextMT&>(ResourceBase::GetContextBase());
    }

    const id<MTLBuffer>& GetUploadSubresourceBuffer(const Resource::SubResource& sub_resource)
    {
        META_FUNCTION_TASK();
        const Data::Index sub_resource_raw_index = sub_resource.GetIndex().GetRawIndex(ResourceBase::GetSubresourceCount());
        m_upload_subresource_buffers.resize(sub_resource_raw_index + 1);

        id<MTLBuffer>& mtl_upload_subresource_buffer = m_upload_subresource_buffers[sub_resource_raw_index];
        if (!mtl_upload_subresource_buffer || mtl_upload_subresource_buffer.length != sub_resource.GetDataSize())
        {
            const id<MTLDevice>& mtl_device = GetContextMT().GetDeviceMT().GetNativeDevice();
            mtl_upload_subresource_buffer = [mtl_device newBufferWithBytes:sub_resource.GetDataPtr()
                                                                    length:sub_resource.GetDataSize()
                                                                   options:MTLResourceStorageModeShared];
        }
        else
        {
            Data::RawPtr p_resource_data = static_cast<Data::RawPtr>([mtl_upload_subresource_buffer contents]);
            META_CHECK_ARG_NOT_NULL(p_resource_data);
            std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), p_resource_data);
        }
        return mtl_upload_subresource_buffer;
    }

private:
    std::vector<id<MTLBuffer>> m_upload_subresource_buffers;
};

} // namespace Methane::Graphics
