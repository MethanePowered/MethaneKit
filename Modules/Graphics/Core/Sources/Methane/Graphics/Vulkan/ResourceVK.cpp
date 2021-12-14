/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/ResourceVK.cpp
Vulkan implementation of the resource objects.

******************************************************************************/

#include "ResourceVK.h"
#include "BufferVK.h"
#include "TextureVK.h"
#include "SamplerVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

IResourceVK::LocationVK::LocationVK(const Location& location)
    : Location(location)
    , m_vulkan_resource_ref(dynamic_cast<IResourceVK&>(GetResource()))
{
    META_FUNCTION_TASK();
    const Resource::Type resource_type = GetResource().GetResourceType();
    switch(resource_type)
    {
    case Resource::Type::Buffer:
        m_descriptor_info_var = vk::DescriptorBufferInfo(
            dynamic_cast<BufferVK&>(GetResource()).GetNativeResource(),
            static_cast<vk::DeviceSize>(GetOffset()),
            static_cast<vk::DeviceSize>(GetResource().GetSubResourceDataSize(GetSubresourceIndex()) - GetOffset())
        );
        break;

    case Resource::Type::Texture:
        // TODO: Textures binding is not supported yet
        break;

    case Resource::Type::Sampler:
        // TODO: Samplers binding is not supported yet
        break;
    }
}

} // using namespace Methane::Graphics
