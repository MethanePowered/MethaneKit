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

FILE: Methane/Graphics/Metal/BufferSet.mm
Metal implementation of the buffer-set interface.

******************************************************************************/

#include <Methane/Graphics/Metal/BufferSet.hh>
#include <Methane/Graphics/Metal/Buffer.hh>

#include <Methane/Instrumentation.h>

namespace Methane::Graphics::Rhi
{

Ptr<IBufferSet> IBufferSet::Create(BufferType buffers_type, const Refs<IBuffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::BufferSet>(buffers_type, buffer_refs);
}

} // namespace Methane::Graphics::Rhi

namespace Methane::Graphics::Metal
{

BufferSet::BufferSet(Rhi::BufferType buffers_type, const Refs<Rhi::IBuffer>& buffer_refs)
    : Base::BufferSet(buffers_type, buffer_refs)
    , m_mtl_buffer_offsets(GetCount(), 0U)
{
    META_FUNCTION_TASK();
    const Refs<Rhi::IBuffer>& refs = GetRefs();
    m_mtl_buffers.reserve(refs.size());
    std::transform(refs.begin(), refs.end(), std::back_inserter(m_mtl_buffers),
        [](const Ref<Rhi::IBuffer>& buffer_ref)
        {
           const Buffer& metal_buffer = static_cast<const Buffer&>(buffer_ref.get());
           return metal_buffer.GetNativeBuffer();
        }
    );
}

} // namespace Methane::Graphics::Metal
