/******************************************************************************

Copyright 2019-2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/BufferSet.cpp
Vulkan implementation of the buffer-set interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/BufferSet.h>
#include <Methane/Graphics/Vulkan/Buffer.h>

#include <Methane/Instrumentation.h>

#include <iterator>

namespace Methane::Graphics::Rhi
{

Ptr<IBufferSet> IBufferSet::Create(BufferType buffers_type, const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<Vulkan::BufferSet>(buffers_type, buffer_refs);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Vulkan
{

static std::vector<vk::Buffer> GetVulkanBuffers(const Refs<Rhi::IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    std::vector<vk::Buffer> vk_buffers;
    std::ranges::transform(buffer_refs, std::back_inserter(vk_buffers),
                   [](const Ref<Rhi::IBuffer>& buffer_ref)
                   {
                       const auto& vertex_buffer = dynamic_cast<const Buffer&>(buffer_ref.get());
                       return vertex_buffer.GetNativeResource();
                   }
    );
    return vk_buffers;
}

BufferSet::BufferSet(Rhi::BufferType buffers_type, const Refs<Rhi::IBuffer>& buffer_refs)
    : Base::BufferSet(buffers_type, buffer_refs)
    , m_vk_buffers(GetVulkanBuffers(buffer_refs))
    , m_vk_offsets(m_vk_buffers.size(), 0U)
{ }

} // namespace Methane::Graphics::Vulkan
