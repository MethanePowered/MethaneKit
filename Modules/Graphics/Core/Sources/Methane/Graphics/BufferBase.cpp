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

FILE: Methane/Graphics/BufferBase.cpp
Base implementation of the buffer interface.

******************************************************************************/

#include "BufferBase.h"
#include "DescriptorHeap.h"
#include "ContextBase.h"

#include <Methane/Instrumentation.h>

#include <cassert>

namespace Methane::Graphics
{

BufferBase::BufferBase(ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceNT(Resource::Type::Buffer, settings.usage_mask, context, descriptor_by_usage)
    , m_settings(settings)
{
    if (!m_settings.size)
    {
        throw std::invalid_argument("Can not create buffer of zero size.");
    }
    ITT_FUNCTION_TASK();
}

void BufferBase::SetData(const SubResources& sub_resources)
{
    ITT_FUNCTION_TASK();

    if (sub_resources.empty())
    {
        throw std::invalid_argument("Can not set buffer data from empty sub-resources.");
    }

    uint32_t data_size = 0;
    for(const SubResource& sub_resource : sub_resources)
    {
        if (!sub_resource.p_data || !sub_resource.data_size)
        {
            throw std::invalid_argument("Can not set empty subresource data to buffer.");
        }
        data_size += sub_resource.data_size;
    }

    if (data_size > m_settings.size)
    {
        throw std::runtime_error("Can not set more data (" + std::to_string(data_size) +
                                 ") than allocated buffer size (" + std::to_string(m_settings.size) + ").");
    }
}

std::string Buffer::GetBufferTypeName(Type type) noexcept
{
    ITT_FUNCTION_TASK();
    switch (type)
    {
    case Type::Data:     return "Data";
    case Type::Index:    return "Index";
    case Type::Vertex:   return "Vertex";
    case Type::Constant: return "Constant";
    default:             assert(0);
    }
    return "Unknown";
}

} // namespace Methane::Graphics